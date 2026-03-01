#ifndef __TP_MEM_ALLOC__
#define __TP_MEM_ALLOC__
#include <unistd.h>

typedef struct TraceProvMemoryAllocator {
    void *(*allocator)(size_t size);
    void (*free)(void *ptr);
} TraceProvMemoryAllocator;


void traceprov_set_mem_config(TraceProvMemoryAllocator config);
void *tp_alloc(size_t size);
void tp_free(void *ptr);
void *tp_alloc_0(size_t size);

#define tp_alloc0_object(TYPE) ((TYPE*)tp_alloc_0(sizeof(TYPE)))

#endif