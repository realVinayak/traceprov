#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <chrono>
#include <algorithm>

#include "traceprov.h"
#include "file_utils.h"

#define PRINT_DEBUG(x) std::cout << "[traceprov]: " << '(' << __FILE__ << ',' << __LINE__ << ")\t" << x << "\t" << "ERRNO: " << errno << std::endl

int map_layer_file(int group_number, void **ptr, int file_size){
    char *file_name = get_injected_str(TRACEPROV_MAIN_TRACE_FILE, group_number, NULL);
    if (file_name == NULL) return 1;
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        PRINT_DEBUG("Error opening the group layer file");
        return 1;
    }
    void *temp_ptr = mmap(
        NULL,
        file_size * TRACEPROV_PAGE_SIZE,
        PROT_READ,
        MAP_SHARED,
        fd,
        0
    );

    if (temp_ptr == MAP_FAILED){
        PRINT_DEBUG("Error mmaping group layer file");
        return 1;
    }

    *ptr = temp_ptr;
    return 0;
}

int map_traceprov_shared_context(struct traceprov_shared_context *ptr){
    int shared_context_fd = open(TRACEPROV_SHARED_CONTEXT, O_RDONLY);
    if (shared_context_fd < 0){
        PRINT_DEBUG("Error opening the scratch file");
        return 1;
    }

    struct traceprov_shared_context *temp_ptr = (struct traceprov_shared_context *)mmap(
        NULL,
        TRACEPROV_SHARED_CONTEXT_SIZE,
        PROT_READ,
        MAP_SHARED,
        shared_context_fd,
        0
    );

    if (temp_ptr == MAP_FAILED){
        PRINT_DEBUG("Error mapping the scratch file");
        return 1;
    }

    PRINT_DEBUG("Map shared context succesful!");

    memcpy(ptr, temp_ptr, sizeof(struct traceprov_shared_context));

    close(shared_context_fd);

    return 0;
}

int main(int argc, char *argv[]){

    int layer_number = 1;

    int arg_index = 1;

    while (arg_index < argc){
        // Parse out layer number
        if (strcmp(argv[arg_index], "-l") == 0){
            layer_number = atoi(argv[arg_index+1]);
        }
        arg_index += 2;
    }

    std::cout << "Using layer: " << layer_number << std::endl;

    struct traceprov_shared_context context;
    if (map_traceprov_shared_context(&context)){
        return 1;
    }


    const int group_layer_number = layer_number + 1;

    const local_context *main_worker_context = &context.local_contexts[context.main_worker_id];
    const traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[layer_number - 1];
    const traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];

    auto present_groups = new std::vector<int64>;

    // Need to mmap the group layer file now. We're going to go mmap the entire file (because we can't have data
    // past the file contents).

    void *group_layer_ptr;
    void *forward_row;

    if (map_layer_file(group_layer_number, &group_layer_ptr, group_layer->size)){
        PRINT_DEBUG("Error opening group layer file");
        return 1;
    }

    for (int64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++){
        const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
        if (gr->in_result){
            present_groups->push_back(group_idx + 1);
        }
    }

    if (map_layer_file(layer_number, &forward_row, main_trace_layer->size)){
        PRINT_DEBUG("Error opening main trace file");
        return 1;
    }

    const int64 gap = ((uint64)main_trace_layer->current_row - (uint64)main_trace_layer->last_mapping);
    assert(gap >= 0);

    // Now, figure out what the last mapped region will have been (or the starting address of it.)
    const int64 infered_gap = main_trace_layer->size == 1 ? 0 : (main_trace_layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
    const void *final_row = (void*)((uint64)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);

    std::sort(present_groups->begin(), present_groups->end());
    std::vector<int64> ** filtered_rows = (std::vector<int64> **)malloc(sizeof(std::vector<int64> *)*(main_trace_layer->num_pk_records));
    for (int key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++) filtered_rows[key_idx] = new std::vector<int64>;
    int iters_made = 0;
    while (forward_row < final_row){
        // Emulate the padding.
        forward_row = (void*)((uint64)main_trace_layer->record_padding + (uint64)forward_row);
        int64 mod_record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row*)forward_row), 0);
        if (mod_record_key == 1940144){
            std::cout << "Considering previous.";
        }
        if (std::binary_search(present_groups->begin(), present_groups->end(), ((struct trace_file_forward_row*)forward_row)->group_count)){
            for (int key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++){
                int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row*)forward_row), key_idx);

                filtered_rows[key_idx]->push_back(record_key);
            }
        }
        iters_made++;
        forward_row = (void*)GET_PK_FROM_ROW(((struct trace_file_forward_row*)forward_row), main_trace_layer->num_pk_records);
    }

    std::cout << "made iters: " << iters_made << std::endl;

    std::sort(filtered_rows[0]->begin(), filtered_rows[0]->end());

    std::cout << "Count of filtered: " << filtered_rows[0]->size() << std::endl;
    // for (int i = 0; i < filtered_rows[0]->size(); i++){
    //     std::cout << filtered_rows[0]->at(i) << "," << std::endl;
    // }
    return 0;
}