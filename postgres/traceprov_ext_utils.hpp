#include <vector>
#include <chrono>

extern "C" {
    #include "traceprov.h"
    #include "nodes/pg_list.h"
    #include "traceprov_utils.h"
    int map_traceprov_shared_context(struct traceprov_shared_context *ptr);
    int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size);


    std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32 worker_count);
    List *traceprov_set_at_offset_int(List *input_list, const uint32 offset, const int value);

    // this is fine, even within the same "scope", because they can be separated by {....}
    #define TP_EVALUATE_START() const auto evaluate_start = std::chrono::high_resolution_clock::now()
    #define TP_EVALUATE_END() const auto evaluate_end = std::chrono::high_resolution_clock::now()
    // This is an expression, since callers may want to do something else with it (assign and then log and then push into some vectorksz)
    #define TP_EVALUATE_DURATION() (std::chrono::duration_cast<std::chrono::microseconds>(evaluate_end - evaluate_start).count())
}