
#include "traceprov.hpp"
#include "utils.hpp"
#include <unistd.h>
#include "duckdb.hpp"
#include <string.h>
#include "file_utils.hpp"
#include <sys/mman.h>
#include "traceprov_settings.hpp"
#include <functional>
#include <thread>
#include "traceprov_derive.hpp"

using namespace duckdb;


// This is done this way because the earlier version (of which SmokedDuck is based)
// does not have the extra info API.
TraceProvAggExtra *g_tp_agg_extra = NULL;

thread_local struct current_context traceprov_current = {
    .my_worker_id = 0,
    .traceprov_shared_context_fd = -1,
    .shared_context = NULL,
    .local_context = NULL,
    .maximum_local_layer_used = 0,
    .maximum_local_layer_used_copy = 0,
    .page_cache_idx = 0
};

typedef struct TraceProvLogBind {
    std::vector<uint64_t> *cols;
    bool infer_null;
    TraceProvLayerNumber layer_number;
    std::vector<uint8_t> *sizes;
} TraceProvLogBind;


static void bp(){

}

static inline struct traceprov_aggregate_layer *get_layer(const uint32_t layer_number){
    if (layer_number == 0){
        bp();
        elog(ERROR, "Expected to always be called with layer number > 0");
    }
    if (layer_number < TRACEPROV_MAX_LAYER_PER_WORKER){
        return &traceprov_current.local_context->cached_layers[layer_number - 1];
    }
    return &traceprov_current.local_context->layers[layer_number - TRACEPROV_MAX_LAYER_PER_WORKER];
}

static inline void grow_if_full(struct traceprov_aggregate_layer *layer){
    if ((layer->current_row == layer->end_of_memory_zone)){
        if ((grow_layer_file(layer))){
            elog(ERROR, "Received an error when growing trace file.");
        }
    }
}

static int initialize_local_and_layer(
    const uint32_t layer_number,
    const uint32_t key_length,
    const uint32_t record_length,
    const bool set_current_row,
    const bool should_hash,
    const bool can_be_null
){
    int rc = 0;
    if ((rc = initialize_local_context())){
        PRINT_ON_DEBUG("Error initializing local context.");
        return rc;
    }
    if ((rc = initialize_layer_file(
        layer_number,
        key_length,
        record_length,
        set_current_row,
        should_hash,
        can_be_null
    ))){
        PRINT_ON_DEBUG("Error initializing layer file");
    }
    return rc;
}


#define TRACEPROV_GROW_IF_TRUE(layer, cond) do { \
    if ((cond)) { \
        grow_layer_file(layer); \
    } \
} while(0); \

#define TP_APPEND_CHUNK_SIZE(LAYER, CHUNK_SIZE) { \
    grow_if_full(LAYER); \
    *((uint64_t *)LAYER->current_row) = CHUNK_SIZE; \
    LAYER->current_row = INCR_BY_BYTES(LAYER->current_row, sizeof(uint64_t)); \
    LAYER->num_rows++; \
    LAYER->record_count += CHUNK_SIZE; \
} \

#define TRACEPROV_SET_BUCKET_ON_STATE(STATE, LAYER) { \
    const uint64_t original_group_number = ++LAYER->num_groups; \
    const uint64_t bucket = original_group_number % TRACEPROV_BUCKET_COUNT; \
    STATE->state = TRACEPROV_SET_WORKER_ID((TRACEPROV_SET_BUCKET(original_group_number, bucket)), traceprov_current.my_worker_id); \
} \

unique_ptr<FunctionData> shared_bind(ClientContext &context, vector<unique_ptr<Expression>> &arguments, TraceProvAggExtra *extra);

static void traceprov_direct_update_partition(Vector inputs[], AggregateInputData &aggr_input_data, idx_t input_count, Vector &states, idx_t count);

static inline bool find_int_vector(const std::vector<uint32_t> *search, uint32_t value){
    if (search == NULL) return false;
    return std::find(search->begin(), search->end(), value) != search->end();
}

static inline std::vector<uint32_t> *get_stat_columns(const TraceProvLayerNumber layer){
    if (g_tp_agg_extra->stats_collector_map == NULL) return NULL;
    if ((g_tp_agg_extra->stats_collector_map->find(layer) != g_tp_agg_extra->stats_collector_map->end())){
        return g_tp_agg_extra->stats_collector_map->at(layer);
    }
    return NULL;
}


#if TRACEPROV_SD_MODE==0

// Taken from duckdb src.
duckdb::AggregateFunction *GetCAggregateFunction(duckdb_aggregate_function function) {
    return reinterpret_cast<duckdb::AggregateFunction *>(function);
}


inline bool RowIsVisible(idx_t row_idx, duckdb::ColumnDataScanState *scan) {
    return (row_idx < scan->next_row_index && scan->current_row_index <= row_idx);
}

inline sel_t RowOffset(idx_t row_idx, duckdb::ColumnDataScanState *scan) {
    return duckdb::UnsafeNumericCast<sel_t>(row_idx - scan->current_row_index);
}
// Inspired from implementation in mode.cpp, but without any class stuff
// since we don't need that.
void traceprov_window(
    duckdb::AggregateInputData &aggr_input_data,
    const duckdb::WindowPartitionInput &partition,
    duckdb::const_data_ptr_t g_state,
    duckdb::data_ptr_t l_state,
    const duckdb::SubFrames &subframes,
    duckdb::Vector &result,
    duckdb::idx_t rid
){
    if (partition.count == 0){
        elog(INFO, "Skipping window because output is empty!");
        return;
    }
    if (subframes.size() != 1)
        elog(ERROR, "Expected the subframe size to be 1!");

    auto frame = subframes.at(0);
    // elog(INFO, "Start: %ld, End: %ld, RID: %ld, Count: %ld", frame.start, frame.end, rid, partition.count);
    // Don't do anything if the frame end is not the row end.
    if (frame.end < partition.count) return;
    // Now, need to do all the bulk stuff.

    auto scan = new duckdb::ColumnDataScanState();
    auto inputs = partition.inputs;
    inputs->InitializeScan(*scan, partition.column_ids);
    duckdb::DataChunk page;
    inputs->InitializeScanChunk(*scan, page);

    int64_t last_chunk_idx = -1;
    for (idx_t row_id = frame.start; row_id < frame.end; row_id++){
        if(!inputs->Seek(row_id, *scan, page)){
            elog(ERROR, "Expected seek to always be fine!")
        }
        // In this case, we'll have already written  up this chunk.
        // So, continue in this case.
        if ((int64_t)scan->chunk_index == last_chunk_idx) continue;
        // traceprov_update(NULL, reinterpret_cast<duckdb_data_chunk>(&page), NULL);
        last_chunk_idx = scan->chunk_index;
    }
}

#endif

#define TP_NO_EXPECT elog(ERROR, "Didn't expect to be called!")

// This is hacky.
// Because we need to support DuckDB version that don't have bind info API.
class TraceProvBoundFunctionExpression : public Expression {
public:
	static constexpr const ExpressionClass TYPE = ExpressionClass::BOUND_FUNCTION;

public:
    TraceProvBoundFunctionExpression(LogicalType return_type, ScalarFunction bound_function,
                                                 vector<unique_ptr<Expression>> arguments,
                                                 unique_ptr<FunctionData> bind_info, bool is_operator);

    //! The bound function expression
    ScalarFunction function;
    //! List of child-expressions of the function
    vector<unique_ptr<Expression>> children;
    //! The bound function data (if any)
    unique_ptr<FunctionData> bind_info;
    //! Whether or not the function is an operator, only used for rendering
    bool is_operator;

public:
    bool IsVolatile() {
        TP_NO_EXPECT;
        return false;
    };
    bool IsConsistent() {
        TP_NO_EXPECT;
        return false;
    }
    bool IsFoldable() const {
        TP_NO_EXPECT;
        return false;
    }
    string ToString() const {
        TP_NO_EXPECT;
        return "";
    };
    bool PropagatesNullValues() const {
        TP_NO_EXPECT;
        return false;
    };
    hash_t Hash() const {
        TP_NO_EXPECT;
        return 0;
    };
    bool Equals(const BaseExpression &other) const {
        TP_NO_EXPECT;
        return false;
    };

    unique_ptr<Expression> Copy() {
        TP_NO_EXPECT;
        return nullptr;
    };
    void Verify() const {
        TP_NO_EXPECT;
    };

    void Serialize(Serializer &serializer) const {
        TP_NO_EXPECT;
    };
    static unique_ptr<Expression> Deserialize(Deserializer &deserializer){
        TP_NO_EXPECT;
    }
};


TraceProvBoundFunctionExpression::TraceProvBoundFunctionExpression(LogicalType return_type, ScalarFunction bound_function,
                                                 vector<unique_ptr<Expression>> arguments,
                                                 unique_ptr<FunctionData> bind_info, bool is_operator)
    : Expression(ExpressionType::BOUND_FUNCTION, ExpressionClass::BOUND_FUNCTION, std::move(return_type)),
      function(std::move(bound_function)), children(std::move(arguments)), bind_info(std::move(bind_info)),
      is_operator(is_operator) {
	
}
void traceprov_reinit_state_direct(DataChunk &input, ExpressionState &state, Vector &result){

    traceprov_reset_local();

    traceprov_reinit_counter++;
    char *traceprov_data_dir = (char *)malloc(sizeof(char)*512);
    memset(traceprov_data_dir, 0, sizeof(char)*512);
    sprintf(traceprov_data_dir, TRACE_PROV_DIR, DataDir);
    int rc = 0;
    #if TRACEPROV_SD_MODE==0
    uint64_t* result_data = FlatVector::GetDataUnsafe<uint64_t>(result);
    #else
    uint64_t* result_data = (uint64_t*)FlatVector::GetData<void>(result);
    #endif
    if ((rc = create_dir_if_not_exists(traceprov_data_dir, (S_IRWXU | S_IRGRP | S_IXGRP)))){
        elog(ERROR, "Error creating traceprovdir!");
    }
    if ((rc = remove_files_from_dir(traceprov_data_dir))){
        elog(ERROR, "Error removing files from traceprov dir!");
    }
    result_data[0] = 0;
    free(traceprov_data_dir);
}

ScalarFunction *traceprov_create_reinit_state(){
    auto func = new ScalarFunction(
        "reinit_state",
        {},
        LogicalType::BIGINT,
        traceprov_reinit_state_direct
    );
    return func;
}

template <typename T> TraceProvStatistics get_chunk_statistics(void *data, const int row_count){
    uint64_t max_value = 0;
    uint64_t min_value = -1;
    for (int idx = 0; idx < row_count; idx++){
        const uint64_t value = ((T *)data)[idx];
        max_value = MAX(value, max_value);
        min_value = MIN(value, min_value);
    }
    return TraceProvStatistics {
        .is_set = true,
        .min_value = min_value,
        .max_value = max_value
    };
}

void traceprov_log_direct(DataChunk &input, ExpressionState &state, Vector &result){
    auto &function = state.expr.Cast<TraceProvBoundFunctionExpression>();
    auto &bind_info = function.bind_info;
    auto &bind_data = bind_info->Cast<TraceProvAggBind>();
    input.Flatten();
    const idx_t num_rows = input.size();
    const idx_t num_cols = input.ColumnCount();
    if (num_rows == 0) return;
    const TraceProvLayerNumber layer_number = bind_data.layer_number;
    const bool can_be_null = bind_data.infer_null || bind_data.cols != NULL;
    if ((initialize_local_and_layer(layer_number, num_cols - 1, 1, true, false, can_be_null))){
        elog(ERROR, "Error setting up local or layer!");
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);
    auto sizes = bind_data.sizes;
    const std::vector<uint32_t> *stat_columns = get_stat_columns(layer_number);
    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        #if TRACEPROV_SD_MODE==1
        void *col_data = FlatVector::GetData<void>(input.data[col_idx]);
        #else
        void *col_data = FlatVector::GetDataUnsafe<void>(input.data[col_idx]);
        #endif
        // elog(INFO, "Layer: %d, Col Idx: %d, Size: %d", layer_number, col_idx - 1, sizes->at(col_idx - 1));
        const auto col_size = sizes->at(col_idx - 1);
        const uint64_t chunk_size = col_size * num_rows;
        TRACEPROV_GROW_IF_TRUE(main_layer, (((uint64_t)main_layer->current_row + chunk_size) > (uint64_t)main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, chunk_size);
        main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, chunk_size);
        const bool col_needs_stats = find_int_vector(stat_columns, col_idx - 1);
        if (col_needs_stats && traceprov_use_table_stats){
            // Collect stats for all this column.
            TraceProvStatistics stats;
            if (col_size == sizeof(uint32_t)){
                stats = get_chunk_statistics<uint32_t>(col_data, num_rows);
            }else{
                stats = get_chunk_statistics<uint64_t>(col_data, num_rows);
            }
            merge_stats(&main_layer->stats[col_idx - 1], &stats);
        }
    }

    // Append the current size..., yuck.
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(main_layer->rows_layer_number);
    grow_if_full(chunk_size_layer);
    *((uint64_t *)chunk_size_layer->current_row) = num_rows;
    chunk_size_layer->current_row = INCR_BY_BYTES(chunk_size_layer->current_row, sizeof(uint64_t));
    const uint64_t existing_record_count = chunk_size_layer->record_count;
    chunk_size_layer->record_count = existing_record_count + num_rows;
    chunk_size_layer->num_rows++;

    const bool is_bool_return = result.GetType() == LogicalType::BOOLEAN;
    if (is_bool_return){
        #if TRACEPROV_SD_MODE==1
        bool *out_data = FlatVector::GetData<bool>(result);
        #else
        bool *out_data = FlatVector::GetDataUnsafe<bool>(result);
        #endif
        memset(out_data, true, sizeof(bool)*num_rows);
    }else{
        const uint64_t start_idx = chunk_size_layer->record_count;
        #if TRACEPROV_SD_MODE==1
        uint64_t *out_data = FlatVector::GetData<uint64_t>(result);
        #else
        uint64_t *out_data = FlatVector::GetDataUnsafe<uint64_t>(result);
        #endif
        memset(out_data, true, sizeof(bool)*num_rows);
        for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
            // This way, the final output of the log is unique (used for point queries)
            // Technically, this can be optimized a lot more (only do this for )
            out_data[row_idx] = (TRACEPROV_SET_WORKER_ID((row_idx + start_idx), traceprov_current.my_worker_id));
        }
    }

    if (can_be_null){
        // If some values can be null, need to look at the validity vectors for it.
        // I don't like this a bit. The DuckDB API needs to be better wrapped so this can be handled more easily.
        // TODO: Handle this better when rewriting this as an extension. Ugh.
        // I guess this specific place doesn't cause _that_ much of a perfformance penalty. The place where it matters more
        // at elast for TPC-H is the aggregate. Where we optimize the crap out of this anyways. Ugh, still icky.
        struct traceprov_aggregate_layer *null_layer = get_layer(main_layer->null_layer_number);
        const uint64_t current_chunk_idx = chunk_size_layer->num_rows;

        std::vector<uint64_t> validity_to_append;
        validity_to_append.reserve(num_cols);
        const uint64_t validity_size = ((num_rows - 1) / 64) + 1;
        for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
            if (bind_data.cols){
                const bool is_nullable = std::find(bind_data.cols->begin(), bind_data.cols->end(), col_idx - 1) != bind_data.cols->end();
                if (!is_nullable) continue;
            }
            auto validity = FlatVector::Validity(input.data[col_idx]);
            if (validity.AllValid()) continue;
            validity_to_append.push_back(col_idx);
        }
        TRACEPROV_GROW_IF_TRUE(null_layer, (((uint64_t)null_layer->current_row + 2*sizeof(uint64_t)) > (uint64_t)null_layer->end_of_memory_zone));
        ((uint64_t*)null_layer->current_row)[0] = current_chunk_idx;
        ((uint64_t*)null_layer->current_row)[1] = validity_to_append.size();
        null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, 2*sizeof(uint64_t));
        for (auto col_idx : validity_to_append)
        {
            auto validity_mask = FlatVector::Validity(input.data[col_idx]);
            uint64_t *validity = validity_mask.GetData();
            // Need to write this column vector's validity.
            // We already know what the size of this vector is (so don't need to write it again)


            // Need to write out the col idx where we say the null.
            // We have validity_size + 1 because also need to write out the column that has null values.
            TRACEPROV_GROW_IF_TRUE(null_layer, (((uint64_t)null_layer->current_row + (validity_size + 1)*sizeof(uint64_t)) > (uint64_t)null_layer->end_of_memory_zone));
            ((uint64_t*)null_layer->current_row)[0] = col_idx - 1;
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t));
            memcpy(null_layer->current_row, validity, sizeof(uint64_t)*validity_size);
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t)*validity_size);
        }
    }
}


unique_ptr<FunctionData> traceprov_log_direct_bind(ClientContext &context, ScalarFunction &bound_function, vector<unique_ptr<Expression>> &arguments){
    return shared_bind(context, arguments, g_tp_agg_extra);
}

#define TRACEPROV_DUCKDB_LOG_FUNC_NAME          "traceprov_log_entry_%d"
#define TRACEPROV_DUCKDB_VOLATILE_LOG_FUNC_NAME "traceprov_log_entry_volatile_%d"
#define TRACEPROV_DUCKDB_BOOLEAN_LOG_FUNC_NAME "traceprov_log_entry_bool_%d"


ScalarFunction **traceprov_create_log_function(const uint32_t num_args, const bool is_volatile, TraceProvNullMap *null_map, const bool is_boolean){
    ScalarFunction** funcs = (ScalarFunction **)malloc(sizeof(ScalarFunction *) * num_args);
    for (uint32_t idx = 0; idx < num_args; idx++){
        char func_name[256] = {0};
        char *func_name_str = "";
        const bool is_boolean_return = is_volatile || is_boolean;
        if (is_volatile){
            func_name_str = TRACEPROV_DUCKDB_VOLATILE_LOG_FUNC_NAME;
        } else if (is_boolean){
            func_name_str = TRACEPROV_DUCKDB_BOOLEAN_LOG_FUNC_NAME;
        } else {
            func_name_str = TRACEPROV_DUCKDB_LOG_FUNC_NAME;
        }
        sprintf(func_name, func_name_str, idx + 1);
        auto func = new ScalarFunction("", {}, duckdb::LogicalType::INVALID, nullptr, nullptr);
        func->name = func_name;
        if (is_boolean_return){
            func->return_type = duckdb::LogicalType::BOOLEAN;
        }else{
            func->return_type = duckdb::LogicalType::UBIGINT;
        }
        func->arguments.push_back(duckdb::LogicalType::INTEGER);
        for (uint32_t arg_idx = 0; arg_idx < idx + 1; arg_idx++){
            func->arguments.push_back(duckdb::LogicalType::ANY);
        }
        func->function = traceprov_log_direct;
        func->bind = traceprov_log_direct_bind;
        if (is_volatile){
            func->stability = FunctionStability::VOLATILE;
        }else{
            func->stability = FunctionStability::CONSISTENT_WITHIN_QUERY;
        }
        func->null_handling = duckdb::FunctionNullHandling::SPECIAL_HANDLING;
        funcs[idx] = func;
    }
    return funcs;
}

#if TRACEPROV_SD_MODE==1

static idx_t traceprov_direct_state_size() {
    return sizeof(struct traceprov_agg_context);
}

static void traceprov_direct_state_init( data_ptr_t state) {
    auto state_ptr = reinterpret_cast<struct traceprov_agg_context *>(state);
    memset(state_ptr, 0, sizeof(struct traceprov_agg_context));
}

#else
static idx_t traceprov_direct_state_size(const AggregateFunction &function) {
    return sizeof(struct traceprov_agg_context);
}

static void traceprov_direct_state_init(const AggregateFunction &function, data_ptr_t state) {
    auto state_ptr = reinterpret_cast<struct traceprov_agg_context *>(state);
    memset(state_ptr, 0, sizeof(struct traceprov_agg_context));
}
#endif

unique_ptr<FunctionData> shared_bind(ClientContext &context, vector<unique_ptr<Expression>> &arguments, TraceProvAggExtra *extra){
    if (!arguments[0]->IsScalar()){
        elog(ERROR, "Expected the first argument to always be scalar.!");
    }
    Value layer_number_value = ExpressionExecutor::EvaluateScalar(context, *arguments[0], false);
    const uint32_t layer_number = layer_number_value.GetValue<uint32_t>();
    if (layer_number == 0){
        elog(ERROR, "Expected layer number to be > 0");
    }
    std::vector<uint8_t> *sizes = new std::vector<uint8_t>;
    const idx_t num_args = arguments.size();
    for (idx_t arg_idx = 1; arg_idx < num_args; arg_idx++){
        const auto return_type = arguments[arg_idx]->return_type;
        uint8_t arg_size = 0;
        if (return_type == LogicalType::UINTEGER || return_type == LogicalType::INTEGER){
            arg_size = sizeof(uint32_t);
        }else{
            arg_size = sizeof(uint64_t);
        }
        sizes->push_back(arg_size);
    }
    std::vector<uint64_t> *null_cols = NULL;
    if (extra->null_map != NULL){
        const bool has_null_cols = extra->null_map->find(layer_number) != extra->null_map->end();
        if (has_null_cols){
            auto cols = extra->null_map->at(layer_number);
            if (cols->size() > 0){
                null_cols = cols;
            }
        }
    }
    bool will_hash = false;
    if (extra->partition_layers && traceprov_use_partition_in_agg){
        for (auto partition_layer_item: *extra->partition_layers){
            if (partition_layer_item.layer == layer_number){
                will_hash = true;
                break;
            }
        }
    }

    auto bind_ptr = make_uniq<TraceProvAggBind>(
        layer_number,
        sizes,
        traceprov_assume_null,
        null_cols,
        will_hash
    );
    return bind_ptr;
}

unique_ptr<FunctionData> traceprov_direct_bind(ClientContext &context, AggregateFunction &function, vector<unique_ptr<Expression>> &arguments){
    return shared_bind(context, arguments, g_tp_agg_extra);
}

static void traceprov_direct_update(Vector inputs[], AggregateInputData &aggr_input_data, idx_t input_count, Vector &states, idx_t count){
    const idx_t num_rows = count;
    if (num_rows == 0) return;

    auto &bind_data = aggr_input_data.bind_data->Cast<TraceProvAggBind>();
    if (bind_data.should_hash){
        return traceprov_direct_update_partition(inputs, aggr_input_data, input_count, states, count);
    }

    TraceProvAggExtra *extra = g_tp_agg_extra;
    const TraceProvLayerNumber layer_number = bind_data.layer_number;
    const std::vector<uint32_t> *stat_columns = get_stat_columns(layer_number);


    const idx_t orig_num_cols = input_count;
    idx_t num_cols = orig_num_cols;
    
    if (extra->ignore_gn){
        num_cols -= 1;
    }

    const bool can_be_null = bind_data.infer_null || bind_data.cols != NULL;
    if (initialize_local_and_layer(layer_number, num_cols, 1, true, false, can_be_null)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }

    #if TRACEPROV_SD_MODE==1
    struct traceprov_agg_context **agg_contexts = FlatVector::GetData<struct traceprov_agg_context *>(states);
    #else
    struct traceprov_agg_context **agg_contexts = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(states);
    #endif
    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    if (likely(agg_contexts != NULL && !extra->ignore_gn)){
        if (traceprov_use_compact){
            const uint64_t chunk_size = sizeof(uint32_t)*num_rows;
            TRACEPROV_GROW_IF_TRUE(main_layer, (((uint64_t)main_layer->current_row + chunk_size) > (uint64_t)main_layer->end_of_memory_zone));
            for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
                const uint32_t group_count = (uint32_t)agg_contexts[row_idx]->state;
                if (group_count == 0){
                    // Annotate the group with the worker id.
                    agg_contexts[row_idx]->state = TRACEPROV_SET_WORKER_ID((++main_layer->num_groups), traceprov_current.my_worker_id);
                }
                *((uint32_t*)main_layer->current_row) = (uint32_t)agg_contexts[row_idx]->state;
                main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, sizeof(uint32_t));
                if (unlikely(main_layer->mask == 0)){
                    // Extract out the mask.
                    // During reading, we reapply this mask ;)
                    main_layer->mask = agg_contexts[row_idx]->state >> 32;
                }
            }
        } else{
            main_layer->mask = -1;
            const uint64_t chunk_size = sizeof(uint64_t)*num_rows;
            TRACEPROV_GROW_IF_TRUE(main_layer, (((uint64_t)main_layer->current_row + chunk_size) > (uint64_t)main_layer->end_of_memory_zone));
            for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
                const uint32_t group_count = (uint32_t)agg_contexts[row_idx]->state;
                if (group_count == 0){
                    // Annotate the group with the worker id.
                    agg_contexts[row_idx]->state = TRACEPROV_SET_WORKER_ID((++main_layer->num_groups), traceprov_current.my_worker_id);
                }
                *((uint64_t*)main_layer->current_row) = agg_contexts[row_idx]->state;
                main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, sizeof(uint64_t));
            }
        }
    }

    auto sizes = bind_data.sizes;
    for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
        inputs[col_idx].Flatten(count);
        #if TRACEPROV_SD_MODE==1
        void *col_data = FlatVector::GetData<void>(inputs[col_idx]);
        #else
        void *col_data = FlatVector::GetDataUnsafe<void>(inputs[col_idx]);
        #endif
        const auto col_size = sizes->at(col_idx - 1);
        auto compact_chunk_size = (col_size*num_rows);
        const bool needs_stats = find_int_vector(stat_columns, col_idx) && traceprov_use_table_stats;
        if (needs_stats){
            // Done like this for vectorization.
            TraceProvStatistics stats;
            if (col_size == sizeof(uint32_t)){
                stats = get_chunk_statistics<uint32_t>(col_data, num_rows);
            }else {
                stats = get_chunk_statistics<uint64_t>(col_data, num_rows);
            }
            merge_stats(&main_layer->stats[col_idx - 1], &stats);
        }
        TRACEPROV_GROW_IF_TRUE(main_layer, (((uint64_t)main_layer->current_row + compact_chunk_size) > (uint64_t)main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, compact_chunk_size);
        main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, compact_chunk_size);
    }

    // Append the current size..., yuck.
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(main_layer->rows_layer_number);
    TP_APPEND_CHUNK_SIZE(chunk_size_layer, num_rows);

    // Need to figure out if the chunk is null. This is much more easier in this version (than using C-API)
    if (can_be_null){
        struct traceprov_aggregate_layer *null_layer = get_layer(main_layer->null_layer_number);
        const uint64_t current_chunk_idx = chunk_size_layer->num_rows;
        std::vector<uint64_t> validity_to_append;
        validity_to_append.reserve(num_cols);
        const uint64_t validity_size = ((num_rows - 1) / 64) + 1;
        for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
            if (bind_data.cols){
                const bool is_nullable = std::find(bind_data.cols->begin(), bind_data.cols->end(), col_idx - 1) != bind_data.cols->end();
                // This way, we are able to skip checking entries that are guaranteed to be not-nullable.
                if (!is_nullable) continue;
            }
            auto validity = FlatVector::Validity(inputs[col_idx]);
            if (validity.AllValid()) continue;
            validity_to_append.push_back(col_idx);
        }
        TRACEPROV_GROW_IF_TRUE(null_layer, (((uint64_t)null_layer->current_row + 2*sizeof(uint64_t)) > (uint64_t)null_layer->end_of_memory_zone));
        ((uint64_t*)null_layer->current_row)[0] = current_chunk_idx;
        ((uint64_t*)null_layer->current_row)[1] = validity_to_append.size();
        null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, 2*sizeof(uint64_t));
        for (auto col_idx : validity_to_append)
        {
            auto validity = FlatVector::Validity(inputs[col_idx]);
            TRACEPROV_GROW_IF_TRUE(null_layer, (((uint64_t)null_layer->current_row + (validity_size + 1)*sizeof(uint64_t)) > (uint64_t)null_layer->end_of_memory_zone));
            // In this case, we are using the col_idx directly (rather than -1, because the col_idx = 0 will be the group number)
            ((uint64_t*)null_layer->current_row)[0] = col_idx;
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t));
            memcpy(null_layer->current_row, validity.GetData(), sizeof(uint64_t)*validity_size);
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t)*validity_size);
        }
    }
}

#if TRACEPROV_SD_MODE==0
static void traceprov_direct_simple_update(Vector inputs[], AggregateInputData &aggr_input_data, idx_t input_count, data_ptr_t state, idx_t count){
    // Ugh. This is kinda rare to be called (we do this in rare cases, only for window functions)
    // So, it is fine to just expand the state out.
    Vector vec(LogicalType::POINTER, count);
    vec.Flatten(count);
    auto vec_data = FlatVector::GetDataUnsafe<data_ptr_t>(vec);
    for (idx_t i = 0; i < count; i++){
        vec_data[i] = state;
    }
    traceprov_direct_update(inputs, aggr_input_data, input_count, vec, count);
}
#endif

static void traceprov_direct_update_partition(Vector inputs[], AggregateInputData &aggr_input_data, idx_t input_count, Vector &states, idx_t count){
    // The below case is fine, since count < 2048.
    const uint16_t num_rows = (uint16_t)count;
    if (num_rows == 0) return;

    auto &bind_data = aggr_input_data.bind_data->Cast<TraceProvAggBind>();
    const TraceProvLayerNumber layer_number = bind_data.layer_number;
    const idx_t orig_num_cols = input_count;

    const bool can_be_null = bind_data.infer_null || bind_data.cols != NULL;
    if (initialize_local_and_layer(layer_number, orig_num_cols, 1, true, true, can_be_null)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }

    uint32_t cursors[TRACEPROV_BUCKET_COUNT] = {0};
    // Technically this can be optimized away if we're not using table stats, but ugh.
    uint16_t distinct_count[TRACEPROV_BUCKET_COUNT] = {0};
    TraceProvStatistics bucket_stats[TRACEPROV_BUCKET_COUNT] = {0};
    #if TRACEPROV_SD_MODE==1
    struct traceprov_agg_context **agg_contexts = FlatVector::GetData<struct traceprov_agg_context *>(states);
    #else
    struct traceprov_agg_context **agg_contexts = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(states);
    #endif
    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    for (uint16_t row_idx = 0; row_idx < num_rows; row_idx++){
        struct traceprov_agg_context *curr_state = agg_contexts[row_idx];
        const uint32_t group_number = (uint32_t)curr_state->state;
        const bool is_init = group_number == 0;
        if (unlikely(is_init)){
            TRACEPROV_SET_BUCKET_ON_STATE(curr_state, main_layer);
        }
        const uint64_t local_bucket = TRACEPROV_GET_BUCKET(curr_state->state);
        const int64_t cursor = cursors[local_bucket]++;
        const bool is_reverse = (local_bucket & 1) != 0;
        uint16_t *slice_vector = ((uint16_t *)main_layer->slice_vectors[local_bucket]);
        const int slice_idx = is_reverse ? -1*(cursor + 1) : cursor;
        slice_vector[slice_idx] = row_idx;
        if (traceprov_use_table_stats){
            bucket_stats[local_bucket].max_value = MAX(bucket_stats[local_bucket].max_value, curr_state->state);
            bucket_stats[local_bucket].min_value = MAX(bucket_stats[local_bucket].min_value, curr_state->state);
            if (unlikely(is_init)){
                ++distinct_count[local_bucket];
            }
        }
    }

    // Technically, validity should be handled by slicing the vector.
    // But this can get expensive. To crudely check whether we need to log null values,
    // we check the original vector (same as before).
    // Then, we log the validity for each row (without checking).
    // Basically, we can get false negatives (where value is not null, but we predict it is).
    // In those cases, we still incdicate that the value is valid. So, it doesn't cause any issues during inference.

    std::vector<uint64_t> validity_to_append;
    if (can_be_null){
        validity_to_append.reserve(orig_num_cols);
        for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
            if (bind_data.cols){
                const bool is_nullable = std::find(bind_data.cols->begin(), bind_data.cols->end(), col_idx - 1) != bind_data.cols->end();
                // This way, we are able to skip checking entries that are guaranteed to be not-nullable.
                if (!is_nullable) continue;
            }
            auto validity = FlatVector::Validity(inputs[col_idx]);
            if (validity.AllValid()) continue;
            validity_to_append.push_back(col_idx);
        }
    }

    for (uint32_t idx = 0; idx < TRACEPROV_BUCKET_COUNT; idx++){
        const uint32_t slice_size = cursors[idx];
        const bool is_reverse = (idx & 1) != 0;
        if (slice_size == 0) continue;
        const uint16_t *slice_vector = ((uint16_t *)main_layer->slice_vectors[idx]);
        struct traceprov_aggregate_layer *current_layer = main_layer;
        if (idx > 0){
            current_layer = get_layer(main_layer->buckets[idx - 1]);
        }
        if (traceprov_use_compact){
            const uint64_t chunk_size = sizeof(uint32_t)*slice_size;
            TRACEPROV_GROW_IF_TRUE(
                current_layer, 
                (((uint64_t)current_layer->current_row + chunk_size) > (uint64_t)current_layer->end_of_memory_zone)
            );
            // Done this way to hopefully auto vectorization.
            if (is_reverse){
                for (int32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint64_t group_count = agg_contexts[slice_vector[-curr_slice_idx - 1]]->state;
                    *((uint32_t*)current_layer->current_row) = group_count;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint32_t));
                    if (unlikely(current_layer->mask == NULL)){
                        current_layer->mask = (group_count >> 32);
                    }
                }
            }else{
                for (uint32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint64_t group_count = agg_contexts[slice_vector[curr_slice_idx]]->state;
                    *((uint32_t*)current_layer->current_row) = group_count;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint32_t));
                    if (unlikely(current_layer->mask == NULL)){
                        current_layer->mask = (group_count >> 32);
                    }
                }
            }
        }else{
            const uint64_t chunk_size = sizeof(uint64_t)*slice_size;
            TRACEPROV_GROW_IF_TRUE(
                current_layer, 
                (((uint64_t)current_layer->current_row + chunk_size) > (uint64_t)current_layer->end_of_memory_zone)
            );
            // Done this way to hopefully auto vectorization.
            if (is_reverse){
                for (int32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint64_t group_count = agg_contexts[slice_vector[-curr_slice_idx - 1]]->state;
                    *((uint64_t*)current_layer->current_row) = group_count;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint64_t));
                    if (unlikely(current_layer->mask == NULL)){
                        current_layer->mask = (group_count >> 32);
                    }
                }
            }else{
                for (uint32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint64_t group_count = agg_contexts[slice_vector[curr_slice_idx]]->state;
                    *((uint64_t*)current_layer->current_row) = group_count;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint64_t));
                    if (unlikely(current_layer->mask == NULL)){
                        current_layer->mask = (group_count >> 32);
                    }
                }
            }
        }

        auto sizes = bind_data.sizes;
        const std::vector<uint32_t> *stat_columns = get_stat_columns(layer_number);
        for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
            inputs[col_idx].Flatten(count);
            #if TRACEPROV_SD_MODE==1
            void *col_data = FlatVector::GetData<void>(inputs[col_idx]);
            #else
            void *col_data = FlatVector::GetDataUnsafe<void>(inputs[col_idx]);
            #endif
            const uint8_t curr_column_size = sizes->at(col_idx - 1);
            const uint32_t compact_chunk_size = (curr_column_size*slice_size);
            const bool needs_stats = find_int_vector(stat_columns, col_idx) && traceprov_use_table_stats;
            TRACEPROV_GROW_IF_TRUE(
                current_layer,
                (((uint64_t)current_layer->current_row + compact_chunk_size) > (uint64_t)current_layer->end_of_memory_zone)
            );
            uint64_t max_value = 0;
            uint64_t min_value = -1;
            if (curr_column_size == sizeof(uint32_t)){
                for (int32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint16_t row_idx = (is_reverse ? (slice_vector[-curr_slice_idx - 1]) : slice_vector[curr_slice_idx]);
                    const uint32_t value = ((uint32_t *)col_data)[row_idx];
                    if (needs_stats){
                        max_value = MAX(value, max_value);
                        min_value = MIN(value, min_value);
                    }
                    *((uint32_t *)current_layer->current_row) = value;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint32_t));
                }
            }else{
                for (int32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    const uint16_t row_idx = (is_reverse ? (slice_vector[-curr_slice_idx - 1]) : slice_vector[curr_slice_idx]);
                    const uint64_t value = ((uint64_t *)col_data)[row_idx];
                    if (needs_stats){
                        max_value = MAX(value, max_value);
                        min_value = MIN(value, min_value);
                    }
                    *((uint64_t *)current_layer->current_row) = value;
                    current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint64_t));
                }
            }
            if (needs_stats){
                TraceProvStatistics local_stats = {
                    .is_set = true,
                    .min_value = min_value,
                    .max_value = max_value
                };
                merge_stats(&current_layer->stats[col_idx - 1], &local_stats);
            }
        }
        if (traceprov_use_table_stats){
            // Also adjust the distinct layer count.
            // We, here, misuse the record count because otherwise we'll heavily overstimate distinct count for the main layer.
            current_layer->record_count += distinct_count[idx];
            merge_stats(&current_layer->stats[orig_num_cols], &bucket_stats[idx]);
        }
        struct traceprov_aggregate_layer *chunk_size_layer = get_layer(current_layer->rows_layer_number);
        TP_APPEND_CHUNK_SIZE(chunk_size_layer, slice_size);

        if (validity_to_append.size()){
            // Need to handle NULL values.
            // We don't bother check if the values for this partition is null (too expensive).
            // We just write 1 for valids, 0 for invalids.
            struct traceprov_aggregate_layer *null_layer = get_layer(current_layer->null_layer_number);
            const uint64_t current_chunk_idx = chunk_size_layer->num_rows;
            ((uint64_t*)null_layer->current_row)[0] = current_chunk_idx;
            ((uint64_t*)null_layer->current_row)[1] = validity_to_append.size();
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, 2*sizeof(uint64_t));
            const uint64_t validity_size = ((slice_size - 1) / 64) + 1;
            for (auto col_idx: validity_to_append){
                auto validity = FlatVector::Validity(inputs[col_idx]);
                TRACEPROV_GROW_IF_TRUE(null_layer, (((uint64_t)null_layer->current_row + (validity_size + 1)*sizeof(uint64_t)) > (uint64_t)null_layer->end_of_memory_zone));
                ((uint64_t*)null_layer->current_row)[0] = col_idx;
                null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t));
                uint64_t *write_null_data = (uint64_t*)null_layer->current_row;
                uint64_t *validity_data = validity.GetData();
                for (int32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                    uint16_t orig_row_idx = 0;
                    if (is_reverse){
                        orig_row_idx = slice_vector[-curr_slice_idx - 1];
                    }else{
                        orig_row_idx = slice_vector[curr_slice_idx];
                    }
                    const uint16_t block_idx = orig_row_idx / 64;
                    const uint16_t inner_block_idx = orig_row_idx % 64;
                    const uint64_t row_is_valid = (uint64_t)((validity_data[block_idx] & (1 << inner_block_idx)) != 0);
                    const uint16_t write_block_idx = curr_slice_idx / 64;
                    const uint16_t write_inner_block_idx = curr_slice_idx % 64;
                    validity_data[write_block_idx] = validity_data[write_block_idx] | (row_is_valid << write_inner_block_idx);
                }
                null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t)*validity_size);
            }
        }
    }
}

static void traceprov_direct_combine(Vector &state, Vector &combined, AggregateInputData &aggr_input_data, idx_t count){
    state.Flatten(count);
    auto &bind_data = aggr_input_data.bind_data->Cast<TraceProvAggBind>();
    auto extra = g_tp_agg_extra;
    // If we're ignoring group numbers, don't do anything.
    if (extra->ignore_gn){
        return;
    }

    #if TRACEPROV_SD_MODE==1
    struct traceprov_agg_context **source_states = FlatVector::GetData<struct traceprov_agg_context *>(state);
    struct traceprov_agg_context **target_states = FlatVector::GetData<struct traceprov_agg_context *>(combined);
    #else
    struct traceprov_agg_context **source_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(state);
    struct traceprov_agg_context **target_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(combined);
    #endif

    if (initialize_local_context() != 0){
        elog(ERROR, "Error setting up local context!");
    }

    const uint32_t layer_number = bind_data.layer_number;
    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    uint32_t combined_layer_number = 0;
    // it is entirely possible that we see not combined aggs for the same layer.
    // For those cases, need to check if we set the combined in the main layer.
    if (((combined_layer_number = main_layer->combined_aggregate_layer_number) == 0)){
        main_layer->combined_aggregate_layer_number = ++traceprov_current.maximum_local_layer_used;
        combined_layer_number =  main_layer->combined_aggregate_layer_number;
    }

    if(initialize_local_and_layer(combined_layer_number, 2, 1, true, false, 0)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }
    struct traceprov_aggregate_layer *combined_layer = get_layer(combined_layer_number);
    if (unlikely(combined_layer->combined_aggregate_layer_number == 0)){
        // It is, actually, entirely possible that the combiner thread doesn't do
        // any its local work. In that case, the main layer won't be set up.
        // That's fine, but we cannot use "is_leader_layer" to detect if aggregate was split.
        // Also, the numbering is arbitrary, so we cannot query it directly.
        // The "best" way is to set combined_aggregate_layer_number, and then query on it.
        combined_layer->combined_aggregate_layer_number = layer_number;
        combined_layer->read_columns_at_once = true;
    }

    const uint64_t chunk_size = sizeof(uint64_t)*count;
    TRACEPROV_GROW_IF_TRUE(combined_layer, (((uint64_t)combined_layer->current_row + 2*chunk_size) > (uint64_t)combined_layer->end_of_memory_zone));

    uint64_t *target_write_ptr = (uint64_t*)combined_layer->current_row;
    uint64_t *source_write_ptr = &(((uint64_t*)combined_layer->current_row)[count]);
    uint64_t target_state_max = 0;
    uint64_t target_state_min = -1;
    uint64_t source_state_max = 0;
    uint64_t source_state_min = -1;
    for (idx_t idx = 0; idx < count; idx++){
        if ( TRACEPROV_GET_IS_COMBINED(target_states[idx]->state) ) {
            if (TRACEPROV_GET_IS_COMBINED(source_states[idx]->state))
                elog(ERROR, "Didn't expect both of the states to be combined...");
        } else{
            if (TRACEPROV_GET_GROUP_COUNT(target_states[idx]->state) != 0){
                elog(ERROR, "Expected group count to be unset!");
            }
        }
        
        bool was_copied = false;

        if (TRACEPROV_GET_GROUP_COUNT(target_states[idx]->state) == 0 && TRACEPROV_GET_GROUP_COUNT(source_states[idx]->state) > 0){
            if (unlikely(TRACEPROV_GET_IS_COMBINED(source_states[idx]->state))){
                elog(ERROR, "Expected source to not be combined in this case!");
            }
            memcpy(target_states[idx], source_states[idx], sizeof(struct traceprov_agg_context));
            was_copied = true;
        }

        const struct traceprov_agg_context *source_state = source_states[idx];
        struct traceprov_agg_context *target_state = target_states[idx];
        const uint64_t original_target_state_value = target_state->state;

        if (!TRACEPROV_GET_IS_COMBINED(target_state->state)){
            // We set the current worker id as the worker, so that it can be used when filtering easily.
            // In this case, we fake the worker as the layer number.
            target_state->state = TRACEPROV_SET_IS_COMBINED(TRACEPROV_SET_LAYER(target_state->state, traceprov_current.my_worker_id));
            if (traceprov_use_table_stats){
                combined_layer->num_groups++;
            }
        }

        #if TRACEPROV_COLLECT_STATS_MODE == 1
        ++target_state->combined_count;
        combined_layer->max_combined_times = MAX(combined_layer->max_combined_times, target_state->combined_count);
        #endif
        target_write_ptr[0] = target_state->state;
        source_write_ptr[0] = source_state->state;
        target_write_ptr++;
        source_write_ptr++;
        if (traceprov_use_table_stats){
            target_state_max = MAX(target_state_max, target_state->state);
            target_state_min = MIN(target_state_min, target_state->state);

            source_state_max = MAX(source_state_max, source_state->state);
            source_state_min = MIN(source_state_min, source_state->state);
        }
    }

    if (traceprov_use_table_stats){
        TraceProvStatistics target_stats = {
            .is_set = true,
            .min_value = target_state_min,
            .max_value = target_state_max
        };

        TraceProvStatistics source_stats = {
            .is_set = true,
            .min_value = source_state_min,
            .max_value = source_state_max
        };
        merge_stats(&combined_layer->stats[0], &target_stats);
        merge_stats(&combined_layer->stats[1], &source_stats);
    }


    // We're not going to look at this anyways.
    combined_layer->current_row = source_write_ptr;
    main_layer->is_leader_layer = true;
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(combined_layer->rows_layer_number);
    TP_APPEND_CHUNK_SIZE(chunk_size_layer, count);
}

static void traceprov_direct_finalize(Vector &state, AggregateInputData &aggr_input_data, Vector &result, idx_t count, idx_t offset){
    state.Flatten(count);
    #if TRACEPROV_SD_MODE==1
    struct traceprov_agg_context **source_states = FlatVector::GetData<struct traceprov_agg_context *>(state);
    uint64_t *result_data = FlatVector::GetData<uint64_t>(result);
    #else
    struct traceprov_agg_context **source_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(state);
    uint64_t *result_data = FlatVector::GetDataUnsafe<uint64_t>(result);
    #endif

    for (idx_t i = 0; i < count; i++){
        result_data[offset + i] = source_states[i]->state;
    }
}

AggregateFunction *traceprov_create_agg_direct_functions(const uint32_t num_args){
    vector<LogicalType> args = {};
    args.push_back(LogicalType(LogicalTypeId::INTEGER));
    for (uint32_t arg_idx = 0; arg_idx < num_args; arg_idx++){
        // This is any (it can technically be only uint64_t or uint32_t, but are able to then compress)
        args.push_back(LogicalType(LogicalTypeId::ANY));
    }
    std::string func_name = "traceprov_agg_key_parallel_offset_" + std::to_string(num_args);
    auto function = new duckdb::AggregateFunction(
        func_name, 
        args,
        duckdb::LogicalType::UBIGINT,
        traceprov_direct_state_size,
        traceprov_direct_state_init,
        traceprov_direct_update,
        traceprov_direct_combine,
        traceprov_direct_finalize,
        FunctionNullHandling::SPECIAL_HANDLING,
        nullptr,
        traceprov_direct_bind
    );
    #if TRACEPROV_SD_MODE==0
    function->window = traceprov_window;
    function->simple_update = traceprov_direct_simple_update;
    #endif
    return function;
}

void traceprov_create_and_register_agg(
    const uint32_t max_num_args,
    duckdb_connection connection,
    TraceProvNullMap *null_map,
    TraceProvPartitionLayers *partition_layers,
    TraceProvStatsCollectorMap *stats_collector_map
){
    auto con = reinterpret_cast<duckdb::Connection *>(connection);
    auto extra = new TraceProvAggExtra;
    extra->ignore_gn = false;
    extra->null_map = null_map;
    extra->partition_layers = partition_layers;
    extra->stats_collector_map = stats_collector_map;
    g_tp_agg_extra = extra;
    for (uint32_t max_arg_limit = 1; max_arg_limit < max_num_args + 1; max_arg_limit++){
        auto agg_func = traceprov_create_agg_direct_functions(max_arg_limit);
        UDFWrapper::RegisterAggrFunction(*agg_func, *con->context);
    }
}
