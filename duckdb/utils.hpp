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
    const TraceProvDuckDbState *state,
    const bool can_be_null
);

extern thread_local struct current_context traceprov_current;
int grow_layer_file(struct traceprov_aggregate_layer *current_layer);
int map_traceprov_shared_context(struct traceprov_shared_context *ptr);
int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size);
void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer);
void traceprov_write_max_used_layer(const uint32_t maximum_layer_used);
std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32_t worker_count);
std::string traceprov_get_layer_info_query();
void traceprov_reset_local();

typedef struct TraceProvPageCacheEntry {
    // Current idx.
    uint32_t idx;
    // Number of pages.
    uint32_t size;
    // cached pages.
    void **pages;
} TraceProvPageCacheEntry;

typedef struct TraceProvPageCache {
    TraceProvPageCacheEntry *initial_entries;
    TraceProvPageCacheEntry *later_entries;
} TraceProvPageCache;

extern TraceProvPageCache g_page_cache;
void traceprov_setup_page_cache(const uint32_t num_threads, const uint32_t page_count = 0);

#define TRACEPROV_PAGE_CACHE_IDX(curr_local_context) (curr_local_context.page_cache_idx)

#endif