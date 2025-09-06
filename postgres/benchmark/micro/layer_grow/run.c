#include "traceprov_utils.h"
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

// Yuk.
// This is done to undef the printf defined in postgres.
#undef printf

#include <stdio.h>
#define TEST_FILE "layer_grow_micro_benchmark.tp"

static inline int exit_on_error(int condition, const char *err){
    if (condition){
        printf("%s", err);
        fflush(stdout);
        exit(condition);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]){

    exit_on_error(argc != 2, "Invalid number of arguments");

    const int final_size_d = atoi(argv[1]);
    const long int final_size = 1UL << final_size_d;

    printf("Using final size: %ld. Using incremennt: %d\n", final_size, TRACEPROV_INCREMENT_TRACE_BY_PG);

    struct traceprov_aggregate_layer layer;
    memset(&layer, 0, sizeof(struct traceprov_aggregate_layer));
    int fd = 0;
    if ((fd = open(TEST_FILE, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION)) < 0){
        printf("Error opening the traceprov file\n");
        return 1;
    }
    
    // Truncate the file initially.
    exit_on_error(ftruncate(fd, 0), "Error truncating to 0");
    exit_on_error(ftruncate(fd, TRACEPROV_PAGE_SIZE), "Error truncating to initial 1 page");
    
    layer.size = 1;
    layer.layer_fd = fd;

    void *trace_ptr = mmap(
        NULL,
        TRACEPROV_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    exit_on_error((trace_ptr == MAP_FAILED), "mmap initial failed");

    long total_time = 0;


    while((layer.size * TRACEPROV_PAGE_SIZE) < final_size){
        const clock_t begin = clock();
        exit_on_error(grow_layer_file(&layer), "Error growing the file");
        const clock_t end = clock();
        total_time += (end - begin);
    }

    printf("Took: %f\n", (double)total_time / CLOCKS_PER_SEC);

}