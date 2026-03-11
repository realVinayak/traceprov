#include "duckdb.hpp"
#include "traceprov_node.hpp"
duckdb_table_function traceprov_create_table_func();
duckdb_table_function traceprov_create_table_offset_func();
typedef struct TraceProvDuckDbGlobalState {
    bool did_initialize;
    std::vector<struct local_context *> *worker_local_contexts;
} TraceProvDuckDbGlobalState;
void reset_global_context();
#if TRACEPROV_SD_MODE == 0
duckdb_scalar_function traceprov_create_table_window_func(const uint64_t num_args, const uint32_t worker_count, std::vector<uint32_t> *expected_layers);
duckdb_scalar_function traceprov_create_read_vector_func();
void *traceprov_get_partition(const uint64_t worker_id, const uint64_t layer_number, const int64_t log_offset, const uint32_t entry_idx, uint64_t *partition_id);
std::vector<TraceProvWorkerLayer> *find_combine_layers_across_workers(
    const TraceProvLayerNumber log_layer_number,
    const std::vector<struct local_context *> *worker_local_contexts
);
duckdb_table_function traceprov_create_infer_table_func();
#endif
