#include <vector>

extern "C" {
    #include "traceprov.h"

    int map_traceprov_shared_context(struct traceprov_shared_context *ptr);
    int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size);
    void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer);

    std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32 worker_count);
}