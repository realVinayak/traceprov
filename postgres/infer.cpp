// This is CPP because it allows some things more easier

#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <list>
#include <chrono>

#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <string.h>

#define PROV_FILE "/var/lib/postgresql/14/main/provfile.prov"
#define SCRATCH_SPACE "/var/lib/postgresql/14/main/scratch.space"

#define SCRATCH_PAGE_SIZE 256

#define GIGA_BYTE 1024 * 1024 * 1024
#define PROV_FILE_SIZE ((long)10 * GIGA_BYTE)

#define PRINT_DEBUG(x) std::cout << "[traceprov]: " << '(' << __FILE__ << ',' << __LINE__ << ")\t" << x << "\t" << "ERRNO: " << errno << std::endl

typedef long int int64;
typedef int int32;

// TODO: Make this part of a header.
struct scratch_space {
    int32 magic_word;
    int worker_count;
    void *trace_file;
    // The count of groups seen.
    int64 group_count;
    // The number of rows written.
    int64 row_count;
};

struct mmap_init_row {
    int64 primary_key;
    int64 group_cnt;
};

struct mmap_later_row {
    int64 in_result;
};

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


int map_trace_file(void **trace_file_ptr){
    int trace_file_fd = open(PROV_FILE, O_RDWR);
    if (trace_file_fd < 0){
        PRINT_DEBUG("Error opening the trace file");
        return 1;
    }

    void *ptr = mmap(
        NULL,
        PROV_FILE_SIZE,
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


void print_scratch(struct scratch_space &ptr){
    std::cout << "WORKER COUNT: " << ptr.worker_count << std::endl;
    std::cout << "TRACE FILE: " << ptr.trace_file << std::endl;
    std::cout << "GROUP COUNT: " << ptr.group_count << std::endl;
    std::cout << "ROW COUNT: " << ptr.row_count << std::endl;
}

void print_init_row(struct mmap_init_row *init_row){
    std::cout << "MMAP INIT ROW {" << std::endl;
    std::cout << "\t" << "[primary_key]:" << "\t" << init_row->primary_key << std::endl;
    std::cout << "\t" << "[group_cnt]:" << "\t" << init_row->group_cnt << std::endl;
    std::cout << "}" << std::endl;
}

void print_later_row(struct mmap_later_row *later_row){
    std::cout << "MMAP LATER ROW {" << std::endl;
    std::cout << "\t" << "[in_result]:" << "\t" << later_row->in_result << std::endl;
    std::cout << "}" << std::endl;
}

int main(int argc, char *argv[]){

    auto start = std::chrono::high_resolution_clock::now();

    auto present_groups = new std::list<int64>;
    auto filtered_rows = new std::list<int64>;

    std::cout << "Beginning inferring" << std::endl;

    struct scratch_space scratch_space_var;
    if (map_scratch_space(&scratch_space_var)){
        return 1;
    }

    print_scratch(scratch_space_var);

    void *trace_file;
    map_trace_file(&trace_file);

    struct mmap_init_row *init_row_iter = (struct mmap_init_row*)(trace_file);

    struct mmap_later_row *init_later_row_iter = ((struct mmap_later_row*)(
        (char*)trace_file + (0+1)*GIGA_BYTE
    ));

    for (int back_iter = scratch_space_var.group_count; back_iter > 0; back_iter--){
        struct mmap_later_row *later_row = init_later_row_iter - back_iter;
        if (later_row->in_result)
            present_groups->push_back(back_iter);
    }

    for (int iter = 0; iter < scratch_space_var.row_count; iter++){
        struct mmap_init_row *row_iter = &init_row_iter[iter];
        for (int group_count: *present_groups){
            if (row_iter->group_cnt == group_count){
                filtered_rows->push_back(row_iter->primary_key);
                break;
            }
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