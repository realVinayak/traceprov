#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "mem_alloc.h"
static TraceProvMemoryAllocator current_mem_config = {
    .allocator = NULL,
    .free = NULL
};

void traceprov_set_mem_config(TraceProvMemoryAllocator config){
    current_mem_config = config;
}

void *tp_alloc(size_t size){
    if (current_mem_config.allocator == NULL)
        EXIT_WITH_MESSAGE("Expected allocator to be set!");
    return current_mem_config.allocator(size);
}

void tp_free(void *ptr){
    if (current_mem_config.free)
        current_mem_config.free(ptr);
}

void *tp_alloc_0(size_t size){
    void *ptr = tp_alloc(size);
    memset(ptr, 0, size);
    return ptr;
}

