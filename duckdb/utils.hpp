#ifndef __TP_UTILS__
#define __TP_UTILS__
#include <stdio.h>
#include <iostream>
#include "traceprov.hpp"
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#define INFO 1
#define ERROR 2

void portable_elog(int level);

#ifndef elog
#define elog(level, ...) do { \
    printf(__VA_ARGS__); \
    printf("%s", "\n"); \
    fflush(stdout); \
    portable_elog(level); \
} while(0); \

#endif

int initialize_file(int fd, const uint32_t *magic_word, size_t size);
int fail_safe_mmap(int fd, size_t size, void **pptr);
int initialize_local_context();

int get_or_create_layer(
    uint32_t layer_number,
    struct traceprov_aggregate_layer **p_layer,
    uint32_t record_width,
    bool set_current_row,
    bool *is_already_present
);

int initialize_layer_file(
    const uint32_t layer_number,
    // Specifies the length of the key of the record.
    const uint32_t key_length,
    // Specifies the length of the record, excluding keys.
    const uint32_t record_length,
    const bool set_current_row,
    const TraceProvDuckDbState *state
);

extern struct current_context traceprov_current;
int grow_layer_file(struct traceprov_aggregate_layer *current_layer);
int map_traceprov_shared_context(struct traceprov_shared_context *ptr);
int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size);
void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer);
#endif