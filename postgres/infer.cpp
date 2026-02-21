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


#include "row.h"

#undef sprintf

#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>


#define MAX_WORKERS 10

#define LENGTH 15

#define PRINT_DEBUG(x) std::cout << "[traceprov]: " << '(' << __FILE__ << ',' << __LINE__ << ")\t" << x << "\t" << "ERRNO: " << errno << std::endl

void print_ids_simple(std::vector<int64> *v);
void print_ids_simple(std::vector<int64> *v1, std::vector<int64> *v2);

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
        addr == NULL ? MAP_SHARED : (MAP_SHARED | MAP_FIXED),
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
    std::cout << "\t" << "TRACE FILE: " << ptr.local_trace_file << std::endl;
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


    char *output_file = NULL;
    int group_no = -1;
    int subq_length = -1;
    char *subq_out_file = NULL;
    int subq_search = 0;

    int arg_index = 1;
    int ignore_group = 0;

    while (arg_index < argc){
        if (strcmp(argv[arg_index], "-f") == 0){
            output_file = argv[arg_index + 1];
        }else if (strcmp(argv[arg_index], "-g") == 0){
            group_no = atoi(argv[arg_index+1]);
        }else if (strcmp(argv[arg_index], "-s.num") == 0){
            subq_length = atoi(argv[arg_index + 1]);
        }else if (strcmp(argv[arg_index], "-s.out") == 0){
            subq_out_file = argv[arg_index + 1];
        }else if (strcmp(argv[arg_index], "-s.find") == 0){
            subq_search = atoi(argv[arg_index + 1]);
        }else if (strcmp(argv[arg_index], "-ig") == 0){
            ignore_group = atoi(argv[arg_index + 1]);
        }
        arg_index += 2;
    }


    std::cout << "Using output file: " << (output_file == NULL ? "NULL" : output_file) << std::endl;
    std::cout << "Using group no: " << group_no << std::endl;


    auto start = std::chrono::steady_clock::now();

    auto present_groups = new std::vector<int64>;

    std::vector<int64> *filtered_rows_all[LENGTH];

    for (int i = 0; i < LENGTH; i++){
        filtered_rows_all[i] = new std::vector<int64>;
    }

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
    struct partial_row * par_row = NULL;

    // Mapping the address directly is pretty sweet, because then we don't need to do much arithmetic.
    map_trace_file(&trace_file, PROV_FILE, PROV_FILE_SIZE, scratch_space_var.trace_file);
    map_trace_file(
        (void**)&par_row, 
        PROV_PARALLEL_TRACE, 
        GIGA_BYTE, 
        scratch_space_var.locals[scratch_space_var.main_worker_id].initial_partial_row
    );

    std::vector<int64> present_second_groups;

    if (
        scratch_space_var.locals[scratch_space_var.main_worker_id].layer_mark != NULL && 
        scratch_space_var.locals[scratch_space_var.main_worker_id].layer_mark != (void*)-1
    ){
        // This means that there are nested groups that we need to look at.
        
        for (
            int second_group_num = 1; 
            second_group_num <= scratch_space_var.locals[scratch_space_var.main_worker_id].second_group_count; 
            second_group_num++){
            struct mmap_later_row *row = (struct mmap_later_row *)scratch_space_var.locals[scratch_space_var.main_worker_id].layer_mark + second_group_num;
            if (row->in_result){
                present_second_groups.push_back(second_group_num);
            }
        }
        
    }

    std::sort(present_second_groups.begin(), present_second_groups.end());


    struct mmap_later_row *init_later_row_iter = ((struct mmap_later_row*)(
        (char*)trace_file + ((long) 2)*GIGA_BYTE
    ));

    int64 max_group_count;
    int64 least_group_count = 0;

    if (!ignore_group){
        if (group_no == -1 || group_no == 1){
            max_group_count = scratch_space_var.locals[scratch_space_var.main_worker_id].group_count;
            least_group_count = scratch_space_var.locals[scratch_space_var.main_worker_id].second_group_count;
        }else{
            max_group_count = scratch_space_var.locals[scratch_space_var.main_worker_id].second_group_count;
        }
    }else{
        max_group_count = scratch_space_var.locals[scratch_space_var.main_worker_id].group_count;
    }

    for (int back_iter = max_group_count; back_iter > least_group_count; back_iter--){
        struct mmap_later_row *later_row = init_later_row_iter - back_iter;
        if (present_second_groups.size() == 0){
            if (later_row->in_result) present_groups->push_back(back_iter);
        }else{
            // We're dealing with nested groups here.
            if (std::binary_search(
                present_second_groups.begin(),
                present_second_groups.end(),
                later_row->in_result
            )){
                present_groups->push_back(back_iter);
            }
        }
    }

    std::cout << "Found present groups." << std::endl;

    struct local_context main_local_context = scratch_space_var.locals[scratch_space_var.main_worker_id];

    // Now, we need to consult the partial file.
    int iters_made = 0;

    std::sort(present_groups->begin(), present_groups->end());

    while (
        par_row < main_local_context.partial_row_ptr
    ){  
        iters_made++;

        if (std::binary_search(present_groups->begin(), present_groups->end(), par_row->global_group_no)){
            if (group_numbers_per_worker[par_row->worker_id] == NULL){
                group_numbers_per_worker[par_row->worker_id] = new std::vector<int64>;
            }
            group_numbers_per_worker[par_row->worker_id]->push_back(par_row->local_group_no);
        }
        // for (int group_count: *present_groups){
        //     if (par_row->global_group_no == group_count){

        //         break;
        //     }
        // }
        par_row++;
    }

    std::cout << "Made iters: " << iters_made << std::endl;
    
    print_per_worker_stats(group_numbers_per_worker);

    if (par_row == NULL || group_numbers_per_worker[scratch_space_var.main_worker_id] == NULL){
        // There is no provenance file here.
        group_numbers_per_worker[scratch_space_var.main_worker_id] = present_groups;
    }

    for (int i = 0; i < MAX_WORKERS; i++){
        std::vector<int64> * local_group_nos = group_numbers_per_worker[i];
        if (local_group_nos == NULL) continue;
        // No point if the size is 0.
        if (local_group_nos->size() == 0) continue;

        std::sort(local_group_nos->begin(), local_group_nos->end());

        struct mmap_init_row *init_row_iter;

        char local_trace_file_name[sizeof(PROV_SUB_FILE) + 4];
        memset(local_trace_file_name, 0, sizeof(local_trace_file_name));
        sprintf(local_trace_file_name, PROV_SUB_FILE, i);

        int should_unmap = 0;
        if (i == scratch_space_var.main_worker_id){
            init_row_iter = (struct mmap_init_row*) scratch_space_var.trace_file;
        }else{
            if(map_trace_file((void **)&init_row_iter, local_trace_file_name, SUB_PROV_FILE_SIZE, scratch_space_var.locals[i].local_trace_file)){
                std::cout << "Error opeing local trace file " << local_trace_file_name << std::endl;
                should_unmap = 1;
            }
        }

        struct mmap_init_row *init_row_iter_previous = init_row_iter;

        while (init_row_iter < scratch_space_var.locals[i].p_init_row){
            
            if(std::binary_search(
                    local_group_nos->begin(), 
                    local_group_nos->end(), 
                    init_row_iter->group_cnt
                )){
                    for (int pk_id = 0; pk_id < init_row_iter->num_records; pk_id++){
                        int64 val = *MMAP_INIT_ROW_PK(init_row_iter, pk_id);
                        filtered_rows_all[pk_id]->push_back(val);
                    }
                }
            init_row_iter = (struct mmap_init_row*) MMAP_INIT_ROW_PK(init_row_iter, init_row_iter->num_records);
        }

        if(should_unmap) munmap(init_row_iter_previous, SUB_PROV_FILE_SIZE);

    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    for (int pk_id = 0; pk_id < LENGTH; pk_id++){
        std::cout << "FILTERED: " << filtered_rows_all[pk_id]->size() << std::endl;
    }

    std::vector<int64> **sub_q_ids_records = NULL;

    if (subq_length > 0){
        sub_q_ids_records = (std::vector<int64> **)malloc(subq_length * sizeof(std::vector<int64> *));
        for (int i = 0; i < subq_length; i++)
            sub_q_ids_records[i] = new std::vector<int64>;

        for (int worker_id = 0; worker_id < MAX_WORKERS; worker_id++){

            int64 *initial_subq_ptr = scratch_space_var.locals[worker_id].initial_subq_pk;

            if (!initial_subq_ptr) continue;

            int64 *subq_pks;

            char subq_file_name[sizeof(PROV_SUBQ_TRACE) + 4];
            memset(subq_file_name, 0, sizeof(subq_file_name));
            sprintf(subq_file_name, PROV_SUBQ_TRACE, worker_id);


            map_trace_file((void **)&subq_pks, subq_file_name, GIGA_BYTE, NULL);

            int subq_record_length = subq_length - subq_search; 
            while (initial_subq_ptr < scratch_space_var.locals[worker_id].subq_pk){

                int should_include = 1;

                if (subq_search){
                    should_include = std::binary_search(
                        filtered_rows_all[0]->begin(),
                        filtered_rows_all[0]->end(),
                        subq_pks[2]
                    );
                }
                
                for (int i = 0; i < subq_length; i++)
                    sub_q_ids_records[i]->push_back(subq_pks[i]);

                subq_pks += subq_length;
                initial_subq_ptr += subq_length;
            }

        }

        for (int subq_i = 0; subq_i < subq_length; subq_i++)
            std::cout << "SUBQ IDS SIZE: " << sub_q_ids_records[subq_i]->size() << std::endl;
    }

    std::cout << "Took: " << duration.count() << " ms" << std::endl;

    if (output_file){

        std::cout << "Writing IDS to " << output_file << std::endl;

        int fd = open(output_file, O_CREAT | O_RDWR, 666);
        if (fd < 0){
            PRINT_DEBUG("Error opening the raw file!");
            return 1;
        }else{
            close(fd);
        }

        std::ofstream output_ids;
        output_ids.open(output_file);

        if (output_ids.is_open()){


            for (int idx = 0; idx < filtered_rows_all[0]->size(); idx++){

                int add_separator = 0;

                for (int pk_idx = 0; pk_idx < LENGTH; pk_idx++){
                    if (filtered_rows_all[pk_idx]->size()){
                        if (add_separator) output_ids << ",";

                        output_ids << filtered_rows_all[pk_idx]->at(idx);
                        add_separator = 1;
                    }
                }

                output_ids << std::endl;
            }

            // output_ids << "NULL";
            output_ids.close();
        }else{
            PRINT_DEBUG("Error opening the ids file!");
        }

    }

    if (subq_out_file){

        std::cout << "Writing subq ids to " << subq_out_file << std::endl;

        int fd = open(subq_out_file, O_CREAT | O_RDWR, 666);

        if (fd < 0){
            PRINT_DEBUG("Error opening the raw file");
            return 1;
        }else{
            close(fd);
        }

        std::ofstream output_ids;
        output_ids.open(subq_out_file);

        for (int idx = 0; idx < sub_q_ids_records[0]->size(); idx++){
            int add_separator = 0;

            for (int pk_idx = 0; pk_idx < subq_length; pk_idx++){
                if (add_separator) output_ids << ",";

                output_ids << sub_q_ids_records[pk_idx]->at(idx);
                add_separator = 1;
            }

            output_ids << std::endl;
        }

        output_ids.close();

    }


    return 0;
}

void print_ids_simple(std::vector<int64> *v){

    std::vector<int64> new_v;

    for (int i = 0; i < v->size(); i++){
        int found = 0;
        for (int val = 0; val < new_v.size(); val++){
            if (new_v[val] == v->at(i)){
                found = 1;
                break;
            }
        }
        if (!found) new_v.push_back(v->at(i));
    }

    for (int i = 0; i < new_v.size(); i++){
        std::cout << new_v.at(i) << " ,";
        if (i % 20 == 0) std::cout << std::endl;
    }
}

void print_ids_simple(std::vector<int64> *v1, std::vector<int64> *v2){
    for (int i = 0; i < v1->size(); i++){
        std::cout << "(" << v1->at(i) << ", " << v2->at(i) << ")" << ",";
        if (i % 20 == 0) std::cout << std::endl;
    }
}