#include "duckdb.hpp"
#include "traceprov_node.hpp"
duckdb_table_function traceprov_create_table_func();
duckdb_table_function traceprov_create_table_offset_func();
duckdb_table_function traceprov_create_table_offset_partition_func();
typedef struct TraceProvIndexContext {
    uint64_t *vector_data;
    uint64_t vector_size;
    std::mutex *index_context_mutex;
    TraceProvLayerNumber root_layer_number;
    List *directly_derivable;
} TraceProvIndexContext;
typedef struct TraceProvDuckDbGlobalState {
    bool did_initialize;
    std::vector<struct local_context *> *worker_local_contexts;
    TraceProvIndexContext *index_context;
} TraceProvDuckDbGlobalState;
extern TraceProvDuckDbGlobalState g_tp_duckdb_state;
void reset_global_context();
#if TRACEPROV_SD_MODE==0
duckdb_scalar_function traceprov_create_table_window_func(const uint64_t num_args, const uint32_t worker_count, std::vector<uint32_t> *expected_layers);
duckdb_scalar_function traceprov_create_read_vector_func();
#endif
void *traceprov_get_row(
    const uint64_t worker_id,
    const uint64_t layer_number,
    const uint64_t log_probe_value,
    std::vector<uint8_t> *sizes
);
std::vector<TraceProvWorkerLayer> *find_combine_layers_across_workers(
    const TraceProvLayerNumber log_layer_number,
    const std::vector<struct local_context *> *worker_local_contexts
);
std::vector<TraceProvWorkerLayer> *find_layers_across_workers(
    const TraceProvLayerNumber log_layer_number,
    const std::vector<struct local_context *> *worker_local_contexts,
    const uint32 expected_layer_width
);
duckdb_table_function traceprov_create_infer_table_func();
void traceprov_attempt_prefaults();
void initialize_global_context();

TraceProvLogSize traceprov_get_total_layer_size();
std::string traceprov_get_layer_stats();
