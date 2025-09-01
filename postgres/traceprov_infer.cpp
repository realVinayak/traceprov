#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <chrono>
#include <algorithm>

#include <fstream>
#include <sstream>
#include <streambuf>


#include "traceprov.h"
#include "file_utils.h"
#include <unistd.h>

#define PRINT_DEBUG(x) std::cout << "[traceprov]: " << '(' << __FILE__ << ',' << __LINE__ << ")\t" << x << "\t" << "ERRNO: " << errno << std::endl

int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size){
    char *file_name = get_bi_injected_str(TRACEPROV_MAIN_TRACE_FILE, layer_number, worker_id, NULL);
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

void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer){
    const int64 gap = ((uint64)layer->current_row - (uint64)layer->last_mapping);
    assert(gap >= 0);
    // Now, figure out what the last mapped region will have been (or the starting address of it.)
    const int64 infered_gap = layer->size == 1 ? 0 : (layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
    void *final_row = (void*)((uint64)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);
    return final_row;
}

int main(int argc, char *argv[]){

    int layer_number = 1;
    char * output_id_file = NULL;

    int arg_index = 1;

    while (arg_index < argc){
        // Parse out layer number
        if (strcmp(argv[arg_index], "-l") == 0){
            layer_number = atoi(argv[arg_index + 1]);
        }
        if (strcmp(argv[arg_index], "-f") == 0){
            output_id_file = argv[arg_index + 1];
        }
        arg_index += 2;
    }

    std::cout << "Using layer: " << layer_number << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    struct traceprov_shared_context context;
    if (map_traceprov_shared_context(&context)){
        return 1;
    }


    const int group_layer_number = layer_number + 1;
    const int partial_group_ln = layer_number + 2;

    const struct local_context *main_worker_context = &context.local_contexts[context.main_worker_id];
    const struct traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[layer_number - 1];
    const struct traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];
    const struct traceprov_aggregate_layer *partial_group_layer = &main_worker_context->cached_layers[partial_group_ln - 1];

    auto present_groups = new std::vector<int64>;

    // Need to mmap the group layer file now. We're going to go mmap the entire file (because we can't have data
    // past the file contents).

    void *group_layer_ptr;
    void *forward_row;
    void *partial_group_row = NULL;

    if (map_layer_file(group_layer_number, context.main_worker_id, &group_layer_ptr, group_layer->size)){
        PRINT_DEBUG("Error opening group layer file");
        return 1;
    }

    for (int64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++){
        const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
        if (gr->in_result){
            present_groups->push_back(group_idx + 1);
        }
    }

    if (map_layer_file(layer_number, context.main_worker_id, &forward_row, main_trace_layer->size)){
        PRINT_DEBUG("Error opening main trace file");
        return 1;
    }

    if (0 == access(
        get_bi_injected_str(TRACEPROV_MAIN_TRACE_FILE, partial_group_ln, main_worker_context->worker_id, NULL),
        F_OK
    )){
        if (map_layer_file(partial_group_ln, context.main_worker_id, &partial_group_row, partial_group_layer->size)){
            PRINT_DEBUG("Error opening the partial trace file");
        }
    }

    if (partial_group_row) PRINT_DEBUG("Using partial trace file");

    void *final_row = get_final_ptr(forward_row, main_trace_layer);

    std::sort(present_groups->begin(), present_groups->end());
    
    std::vector<int64> ** groups_per_worker = (std::vector<int64> **)malloc(sizeof(std::vector<int64>*)*(context.worker_count));
    for (int worker_id = 0; worker_id < context.worker_count; worker_id++) groups_per_worker[worker_id] = new std::vector<int64>;

    if (partial_group_row != NULL){
        // Need to, now, find the rows in the partial file.
        const void *final_partial_group_row_ptr = get_final_ptr(partial_group_row, partial_group_layer);

        while (partial_group_row < final_partial_group_row_ptr){
            partial_group_row = (void*)((uint64)partial_group_layer->record_padding + (uint64)partial_group_row);
            const struct trace_file_partial_row *current_partial_row = (struct trace_file_partial_row *)partial_group_row;
            // Essentially, if the global group number gets found, store the local group number.
            if (std::binary_search(present_groups->begin(), present_groups->end(), current_partial_row->global_group_number)){
                groups_per_worker[current_partial_row->worker_id]->push_back(current_partial_row->local_group_number);
            }

            partial_group_row = (void*)((uint8*)partial_group_row + sizeof(struct trace_file_partial_row));
        }
    }

    for (int worker_id = 0; worker_id < context.worker_count; worker_id++) std::cout << "WORKER: " << worker_id << " GROUPS: "  << groups_per_worker[worker_id]->size() << std::endl;

    std::vector<int64> ** filtered_rows = (std::vector<int64> **)malloc(sizeof(std::vector<int64> *)*(main_trace_layer->num_pk_records));

    for (int key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++) filtered_rows[key_idx] = new std::vector<int64>;

    int iters_made = 0;

    if (partial_group_row == NULL)
        groups_per_worker[context.main_worker_id] = present_groups;


    for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

        std::vector<int64> *local_group_nos = groups_per_worker[worker_id];
        if (local_group_nos->size() == 0) continue;

        std::sort(local_group_nos->begin(), local_group_nos->end());

        const struct local_context *bg_context = &context.local_contexts[worker_id];
        const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[layer_number - 1];

        void *current_forward_row = NULL;
        void *last_forward_row = NULL;
        if (worker_id == context.main_worker_id){
            current_forward_row = forward_row;
        }else{
            void *local_fwd_row = NULL;
            if (map_layer_file(layer_number, worker_id, &current_forward_row, bg_trace_layer->size)){
                PRINT_DEBUG("Error opening the bg trace file");
            }
        }

        const void *current_final_row = get_final_ptr(current_forward_row, bg_trace_layer);

        while (current_forward_row < current_final_row){
            current_forward_row = (void*)((uint64)bg_trace_layer->record_padding + (uint64)current_forward_row);

            if (std::binary_search(local_group_nos->begin(), local_group_nos->end(), ((struct trace_file_forward_row*)current_forward_row)->group_count)){
                for (int key_idx = 0; key_idx < bg_trace_layer->num_pk_records; key_idx++){
                    int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row*)current_forward_row), key_idx);

                    filtered_rows[key_idx]->push_back(record_key);
                }
            }
            iters_made++;
            current_forward_row = (void*)GET_PK_FROM_ROW(((struct trace_file_forward_row*)current_forward_row), bg_trace_layer->num_pk_records);
        }
    }

    for (int pk_id = 0; pk_id < main_trace_layer->num_pk_records; pk_id++){
        std::cout << "FILTERED: " << filtered_rows[pk_id]->size() << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Took: " << duration.count() << " ms" << std::endl;

    if (output_id_file){
        
        std::cout << "Writing IDS to " << output_id_file << std::endl;

        int fd = open(output_id_file, O_CREAT | O_RDWR, 666);
        if (fd < 0){
            PRINT_DEBUG("Error opening the ID file");
            return 1;
        }else{
            close(fd);
        }

        std::ofstream output_ids;
        output_ids.open(output_id_file);

        if (!output_ids.is_open()){
            std::cout << "Error getting the ID file open later";
            return 1;
        }

        for (int record_index = 0; record_index < filtered_rows[0]->size(); record_index++){
            bool add_separator = false;

            for (int key_index = 0; key_index < main_trace_layer->num_pk_records; key_index++){
                if (filtered_rows[key_index]->size() == 0) continue;
                if (add_separator) output_ids << ",";
                output_ids << filtered_rows[key_index]->at(record_index);
                add_separator = true;
            }

            output_ids << std::endl;
        }
        output_ids.close();
    }
    return 0;
}