// This is CPP because it allows some things more easier

#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <list>
#include <vector>
#include <chrono>
#include <algorithm>

#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <string.h>

#include "row.h"

#define MAX_WORKERS 10


#define PRINT_DEBUG(x) std::cout << "[traceprov]: " << '(' << __FILE__ << ',' << __LINE__ << ")\t" << x << "\t" << "ERRNO: " << errno << std::endl

int map_scratch_space(struct scratch_space *ptr){
    int scratch_fd = open(SCRATCH_SPACE, O_RDWR);
    if (scratch_fd < 0){
        PRINT_DEBUG("Error opening the scratch file");
        return 1;
    }

    struct scratch_space *temp_ptr = (struct scratch_space *)mmap(
        NULL, 
        SCRATCH_PAGE_SIZE, 
        PROT_WRITE, 
        MAP_SHARED, 
        scratch_fd, 
        0
    );

    if (temp_ptr == MAP_FAILED){
        PRINT_DEBUG("Error mapping the scratch file");
        return 1;
    }

    PRINT_DEBUG("Map succesful!");

    memcpy(ptr, temp_ptr, sizeof(struct scratch_space));

    close(scratch_fd);

    return 0;
}


int map_trace_file(void **trace_file_ptr, const char *fileName, long int size, void * addr){
    int trace_file_fd = open(fileName, O_RDWR);
    if (trace_file_fd < 0){
        PRINT_DEBUG("Error opening the file: :");
        PRINT_DEBUG(fileName);
        return 1;
    }

    void *ptr = mmap(
        addr,
        size,
        PROT_WRITE,
        MAP_SHARED,
        trace_file_fd,
        0
    );

    if (ptr == MAP_FAILED){
        PRINT_DEBUG("Error mapping the trace file");
        return 1;
    }

    *trace_file_ptr = ptr;
    PRINT_DEBUG("Mapped the trace file successfully");
    return 0;
}

void print_local_context(struct local_context &ptr, void *trace_file){
    std::cout << "LOCAL CONTEXT: {" << std::endl;
    std::cout << "\t" << "WORKER PID: " << ptr.worker_pid << std::endl;
    std::cout << "\t" << "WORKER ID: " << ptr.worker_id << std::endl;
    std::cout << "\t" << "GROUP COUNT: " << ptr.group_count << std::endl;
    std::cout << "\t" << "TRACE FILE: " << ptr.trace_file << std::endl;
    std::cout << "\t" << "INIT ROW: " << ptr.p_init_row << std::endl;
    std::cout << "\t" << "SHOULD PRINT: " << ptr.should_print << std::endl;
    std::cout << "\t" << "PARTIAL ROW: " << ptr.partial_row_ptr << std::endl;
    std::cout << "\t" << "PARTIAL INIT ROW: " << ptr.initial_partial_row << std::endl;
    // std::cout << "\t" << "ROWS COUNT: " << (
    //     (long int)ptr.p_init_row - (long int)((char *)trace_file + PARTITION_SIZE * ptr.worker_id) 
    // ) / sizeof(struct mmap_init_row) << std::endl;
    std::cout << "}" << std::endl;
}

void print_scratch(struct scratch_space &ptr){
    std::cout << "WORKER COUNT: " << ptr.worker_count << std::endl;
    std::cout << "TRACE FILE: " << ptr.trace_file << std::endl;
    // std::cout << "GROUP COUNT: " << ptr.group_count << std::endl;
    // std::cout << "ROW COUNT: " << ptr.row_count << std::endl;
    std::cout << "MAIN WORKER ID: " << (int)ptr.main_worker_id << std::endl;

    for (int i  = 0; i < MAX_WORKERS; i++){
        print_local_context(ptr.locals[i], ptr.trace_file);
    }
}

void print_init_row(struct mmap_init_row *init_row){
    std::cout << "MMAP INIT ROW {" << std::endl;
    std::cout << "\t" << "[num_records]:" << "\t" << init_row->num_records << std::endl;
    std::cout << "\t" << "[group_cnt]:" << "\t" << init_row->group_cnt << std::endl;
    std::cout << "}" << std::endl;
}

void print_later_row(struct mmap_later_row *later_row){
    std::cout << "MMAP LATER ROW {" << std::endl;
    std::cout << "\t" << "[in_result]:" << "\t" << later_row->in_result << std::endl;
    std::cout << "}" << std::endl;
}

void print_per_worker_stats(std::vector<int64> **group_number_per_worker){
    for (int i = 0; i < MAX_WORKERS; i++){
        if (group_number_per_worker[i] == NULL) continue;
        std::cout << "WORKER: " << i << " COUNT: " << group_number_per_worker[i]->size() << std::endl;
    }
}


int main(int argc, char *argv[]){

    auto start = std::chrono::high_resolution_clock::now();

    auto present_groups = new std::vector<int64>;
    auto filtered_rows = new std::vector<int64>;

    std::vector<int64> *group_numbers_per_worker[MAX_WORKERS];

    for (int i = 0; i < MAX_WORKERS; i++){
        group_numbers_per_worker[i] = NULL;
    }

    std::cout << "Beginning inferring" << std::endl;

    struct scratch_space scratch_space_var;
    if (map_scratch_space(&scratch_space_var)){
        return 1;
    }

    print_scratch(scratch_space_var);

    void *trace_file;
    struct partial_row * par_row;

    // Mapping the address directly is pretty sweet, because then we don't need to do much arithmetic.
    map_trace_file(&trace_file, PROV_FILE, PROV_FILE_SIZE, scratch_space_var.trace_file);
    map_trace_file(
        (void**)&par_row, 
        PROV_PARALLEL_TRACE, 
        GIGA_BYTE, 
        scratch_space_var.locals[scratch_space_var.main_worker_id].initial_partial_row
    );


    struct mmap_later_row *init_later_row_iter = ((struct mmap_later_row*)(
        (char*)trace_file + ((long) 2)*GIGA_BYTE
    ));

    int64 max_group_count = scratch_space_var.locals[scratch_space_var.main_worker_id].group_count;

    for (int back_iter = max_group_count; back_iter > 0; back_iter--){
        struct mmap_later_row *later_row = init_later_row_iter - back_iter;
        if (later_row->in_result)
            present_groups->push_back(back_iter);
    }

    struct local_context main_local_context = scratch_space_var.locals[scratch_space_var.main_worker_id];

    // Now, we need to consult the partial file.
    int iters_made = 0;
    while (
        par_row < main_local_context.partial_row_ptr
    ){  
        iters_made++;
        for (int group_count: *present_groups){
            if (par_row->global_group_no == group_count){
                if (group_numbers_per_worker[par_row->worker_id] == NULL){
                    group_numbers_per_worker[par_row->worker_id] = new std::vector<int64>;
                }
                group_numbers_per_worker[par_row->worker_id]->push_back(par_row->local_group_no);
                break;
            }
        }
        par_row++;
    }

    // std::cout << "Made iters: " << iters_made << std::endl;
    
    print_per_worker_stats(group_numbers_per_worker);


    for (int i = 0; i < MAX_WORKERS; i++){
        std::vector<int64> * local_group_nos = group_numbers_per_worker[i];
        if (local_group_nos == NULL) continue;
        // No point if the size is 0.
        if (local_group_nos->size() == 0) continue;

        std::sort(local_group_nos->begin(), local_group_nos->end());
        
        struct mmap_init_row *init_row_iter = (struct mmap_init_row*)((char*)trace_file + PARTITION_SIZE*i);
        while (init_row_iter < scratch_space_var.locals[i].p_init_row){
            
            if(std::binary_search(
                    local_group_nos->begin(), 
                    local_group_nos->end(), 
                    init_row_iter->group_cnt
                )){
                    for (int pk_id = 0; pk_id < init_row_iter->num_records; pk_id++){
                        filtered_rows->push_back(*MMAP_INIT_ROW_PK(init_row_iter, pk_id));
                    }
                }
            init_row_iter = (struct mmap_init_row*) MMAP_INIT_ROW_PK(init_row_iter, init_row_iter->num_records);
        }

    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "NUM FILTERED: " << filtered_rows->size() << std::endl;
    std::cout << "Took: " << duration.count() << " ms" << std::endl;


    // If there are more than 1 arguments, assume that the other is the output file for the IDs.

    if (argc == 2){

        std::cout << "Writing IDS to " << argv[1] << std::endl;

        int fd = open(argv[1], O_CREAT | O_RDWR, 666);
        if (fd < 0){
            PRINT_DEBUG("Error opening the raw file!");
            return 1;
        }else{
            close(fd);
        }

        std::ofstream output_ids;
        output_ids.open(argv[1]);

        if (output_ids.is_open()){
            for (int64 pk: *filtered_rows){
                output_ids << pk << ",";
            }
            output_ids << "NULL";
            output_ids.close();
        }else{
            PRINT_DEBUG("Error opening the ids file!");
        }

    }



    return 0;
}