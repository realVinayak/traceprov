#include "duckdb.hpp"
duckdb_table_function traceprov_create_table_func();
duckdb_table_function traceprov_create_table_offset_func();
typedef struct TraceProvDuckDbGlobalState {
    bool did_initialize;
    std::vector<struct local_context *> *worker_local_contexts;
} TraceProvDuckDbGlobalState;
void reset_global_context();
#if TRACEPROV_SD_MODE == 0
duckdb_scalar_function traceprov_create_table_window_func(const uint64_t num_args, const uint32_t worker_count, std::vector<uint32_t> *expected_layers);
void *traceprov_get_partition(const uint64_t worker_id, const uint64_t layer_number, const int64_t log_offset, int64_t *partition_id);
#endif
