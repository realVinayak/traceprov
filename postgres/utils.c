#include "traceprov_utils.h"
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include "traceprov_settings.h"

int get_error_no(){
    int err_no = errno;
    return err_no;
}

#ifdef TRACEPROV_STANDALONE
#undef PRINT_ON_DEBUG
#define PRINT_ON_DEBUG(...) 0
#undef  elog
#define elog(...) 0
#endif

void print_layer(struct traceprov_aggregate_layer *layer){
    elog(INFO, "traceprov_aggregate_layer {");
    elog(INFO, "\t->num_pk_records: %d", layer->num_pk_records);
    elog(INFO, "\t->last_mapping: %p", layer->last_mapping);
    elog(INFO, "\t->size: %d", layer->size);
    elog(INFO, "\t->current_row: %p", layer->current_row);
    elog(INFO, "\t->num_groups: %ld", layer->num_groups);
    elog(INFO, "\t->num_rows: %ld", layer->num_rows);
    elog(INFO, "\t->layer_number: %d", layer->layer_number);
    elog(INFO, "\t->record_padding: %d", layer->record_padding);
    elog(INFO, "\t->end_of_memory_zone: %p", layer->end_of_memory_zone);
    elog(INFO, "\t->layer_fd: %d", layer->layer_fd);
    elog(INFO, "}");
}

int grow_layer_file(struct traceprov_aggregate_layer *current_layer){
    int rc = 0;
    // In this case, we'd have to grow the file.
    const long int initial_size = current_layer->size;
    // Unmap previous allocation.
    if ((rc = munmap(current_layer->last_mapping, current_layer->last_allocation_size * TRACEPROV_PAGE_SIZE))){
        elog(ERROR, "Error unmaping");
    }
    current_layer->size += TRACEPROV_INCREMENT_TRACE_BY_PG;
    const long int next_size = (current_layer->size) * TRACEPROV_PAGE_SIZE;
    if (unlikely(rc = ftruncate(current_layer->layer_fd, next_size))){
        elog(ERROR, "Error increasing the page size layer: %d", rc);
    }
    // Now, need to create the new mapping.
    void *ptr = mmap(
        NULL,
        TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        current_layer->layer_fd,
        initial_size * TRACEPROV_PAGE_SIZE
    );

    if ((unlikely(ptr == MAP_FAILED))){
        elog(ERROR,
            "Error mmaping incremented trace file. %ld, %ld", 
            TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE,
            initial_size * TRACEPROV_PAGE_SIZE
        );
    }

    // Now, need to some reinitialzation.
    current_layer->end_of_memory_zone = (void*)((TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE) + (char*)ptr);
    current_layer->current_row = ptr;
    // Also set the last mapping.
    current_layer->last_mapping = ptr;
    current_layer->last_allocation_size = TRACEPROV_INCREMENT_TRACE_BY_PG;
    return rc;
}

void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer){
    const uint64 gap = ((uint64)layer->current_row - (uint64)layer->last_mapping);
    assert(gap >= 0);
    // Now, figure out what the last mapped region will have been (or the starting address of it.)
    const uint64 infered_gap = layer->size == layer->initial_allocation_size ? 0 : (layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
    void *final_row = (void*)((uint64)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);
    return final_row;
}