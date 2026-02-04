#include <vector>

extern "C" {
    #include "traceprov.h"
    #include "nodes/pg_list.h"
    #include "traceprov_utils.h"
    int map_traceprov_shared_context(struct traceprov_shared_context *ptr);
    int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size);


    std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32 worker_count);
    List *traceprov_set_at_offset_int(List *input_list, const uint32 offset, const int value);
}