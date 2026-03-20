
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

using namespace duckdb;

thread_local struct current_context traceprov_current = {
    .my_worker_id = 0,
    .traceprov_shared_context_fd = -1,
    .shared_context = NULL,
    .local_context = NULL,
    .maximum_local_layer_used = 0,
    .maximum_local_layer_used_copy = 0,
    .page_cache_idx = 0
};

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#if TRACEPROV_SD_MODE==0
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
    const TraceProvDuckDbState *state,
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
        state,
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

#define TP_UPDATE_SETUP_MAIN(ROW_LAYER_WIDTH) { \
    extra = (TraceProvAggExtra *) duckdb_aggregate_function_get_extra_info(info); \
    orig_num_cols = duckdb_data_chunk_get_column_count(input); \
    num_cols = orig_num_cols; \
    if (extra->ignore_gn) \
        num_cols -= 1; \
    duckdb_vector first_col_vector = duckdb_data_chunk_get_vector(input, 0); \
    layer_number = ((uint32_t*)duckdb_vector_get_data(first_col_vector))[0]; \
    if(initialize_local_and_layer(layer_number, num_cols, ROW_LAYER_WIDTH, true, extra->state, 0)){ \
        elog(ERROR, "Error setting up local or layer!"); \
        return; \
    } \
    agg_contexts = (struct traceprov_agg_context **)states; \
    main_layer = get_layer(layer_number); \
    if (unlikely(extra->size_map->find(layer_number) == extra->size_map->end())) { \
        std::vector<uint8_t> *sizes = new std::vector<uint8_t>; \
        sizes->push_back(sizeof(uint64_t)); \
        for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++) { \
            duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx); \
            void *col_data = (void *)duckdb_vector_get_data(col_vector); \
            auto col_type = duckdb_vector_get_column_type(col_vector); \
            auto logical_type = reinterpret_cast<duckdb::LogicalType *>(col_type); \
            sizes->push_back(logical_type->id() == duckdb::LogicalType::INTEGER ? (sizeof(uint32_t)) : (sizeof(uint64_t))); \
            duckdb_destroy_logical_type(&col_type); \
        } \
        extra->size_map->insert({layer_number, sizes}); \
    } \
} \

#define TP_APPEND_CHUNK_SIZE(LAYER, CHUNK_SIZE) { \
    grow_if_full(LAYER); \
    *((uint64_t *)LAYER->current_row) = CHUNK_SIZE; \
    LAYER->current_row = INCR_BY_BYTES(LAYER->current_row, sizeof(uint64_t)); \
    LAYER->num_rows++; \
    LAYER->record_count += CHUNK_SIZE; \
} \

#define TRACPROV_SET_BUCKET_ON_STATE(STATE, LAYER, EXTRA) { \
    STATE->layer_number = layer_number; \
    const uint64_t original_group_number = ++LAYER->num_groups; \
    const uint64_t bucket = original_group_number % TRACEPROV_BUCKET_COUNT; \
    STATE->group_cnt = TRACEPROV_SET_WORKER_ID((TRACEPROV_SET_BUCKET(original_group_number, bucket)), traceprov_current.my_worker_id); \
} \

// Partition version of update.
// They both share a lot of things.
// But, separating the code still helps
// Common things are put in macros.
void traceprov_update_partition(duckdb_function_info info, duckdb_data_chunk input, duckdb_aggregate_state *states){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);
    PRINT_ON_DEBUG("Number of rows: %ld", num_rows);
    if (num_rows == 0) return;

    TraceProvAggExtra *extra;
    idx_t orig_num_cols, num_cols;
    struct traceprov_agg_context **agg_contexts;
    struct traceprov_aggregate_layer *main_layer;
    uint32_t layer_number;

    TP_UPDATE_SETUP_MAIN(1);

    if (unlikely(agg_contexts == NULL || extra->ignore_gn))
        elog(ERROR, "Invalid options!");
    
    uint32_t cursors[TRACEPROV_BUCKET_COUNT] = {0};

    for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
        struct traceprov_agg_context *curr_state = agg_contexts[row_idx];

        if (unlikely(curr_state->layer_number == 0)){
            TRACPROV_SET_BUCKET_ON_STATE(curr_state, main_layer, extra);
        }

        const uint64_t local_bucket = TRACEPROV_GET_BUCKET(curr_state->group_cnt);
        // This "collects" the row ids that correspond to this bucket.
        const uint64_t cursor = cursors[local_bucket]++;
        uint32_t *slice_vector = ((uint32_t *)(extra->slice_vectors[local_bucket]));
        slice_vector[cursor] = row_idx;
    }

    for (uint32_t idx = 0; idx < TRACEPROV_BUCKET_COUNT; idx++){
        // Nothing to do for this bucket.
        // Will happen when nothing gets hashed into for this bucket.
        const uint32_t slice_size = cursors[idx];
        if (slice_size == 0) continue;
        // avoids the function call overhead. But ig compiler can do it??
        uint32_t *slice_idx = (uint32_t *)extra->slice_vectors[idx];
        // This will be the size of this chunk.
        const uint64_t chunk_size = sizeof(uint64_t)*slice_size;
        struct traceprov_aggregate_layer *current_layer = main_layer;
        if (idx > 0){
            current_layer = get_layer(main_layer->buckets[idx - 1]);
        }
        TRACEPROV_GROW_IF_TRUE(current_layer, ((current_layer->current_row + chunk_size) > current_layer->end_of_memory_zone));
        for (uint32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
            *((uint64_t*)current_layer->current_row) = agg_contexts[slice_idx[curr_slice_idx]]->group_cnt;
            current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint64_t));
        }
        for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
            duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
            uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
            TRACEPROV_GROW_IF_TRUE(current_layer, ((current_layer->current_row + chunk_size) > current_layer->end_of_memory_zone));
            for (uint32_t curr_slice_idx = 0; curr_slice_idx < slice_size; curr_slice_idx++){
                *((uint64_t*)current_layer->current_row) = col_data[slice_idx[curr_slice_idx]];
                current_layer->current_row = INCR_BY_BYTES(current_layer->current_row, sizeof(uint64_t));
            }
        }

        struct traceprov_aggregate_layer *curr_rows_layer = get_layer(current_layer->rows_layer_number);
        TP_APPEND_CHUNK_SIZE(curr_rows_layer, slice_size);
    }
}


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

void traceprov_reinit_state(duckdb_function_info, duckdb_data_chunk input, duckdb_vector output){

    traceprov_reset_local();

    traceprov_reinit_counter++;
    char *traceprov_data_dir = (char *)malloc(sizeof(char)*512);
    memset(traceprov_data_dir, 0, sizeof(char)*512);
    sprintf(traceprov_data_dir, TRACE_PROV_DIR, DataDir);
    int rc = 0;
    uint64_t* result_data = (uint64_t *)duckdb_vector_get_data(output);
    if ((rc = create_dir_if_not_exists(traceprov_data_dir, (S_IRWXU | S_IRGRP | S_IXGRP)))){
        elog(ERROR, "Error creating traceprovdir!");
    }
    if ((rc = remove_files_from_dir(traceprov_data_dir))){
        elog(ERROR, "Error removing files from traceprov dir!");
    }
    result_data[0] = 0;
    free(traceprov_data_dir);
}

duckdb_scalar_function traceprov_create_reinit_state(){
    duckdb_scalar_function func = duckdb_create_scalar_function();
    duckdb_scalar_function_set_name(func, "reinit_state");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
    duckdb_scalar_function_set_return_type(func, type);
    duckdb_destroy_logical_type(&type);
    duckdb_scalar_function_set_function(func, traceprov_reinit_state);
    return func;
}

void traceprov_log(duckdb_function_info info, duckdb_data_chunk input, duckdb_vector output){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);
    const idx_t num_cols = duckdb_data_chunk_get_column_count(input);
    if (num_rows == 0) return;
    TraceProvDuckDbState *tp_duckdb_state = (TraceProvDuckDbState *) duckdb_scalar_function_get_extra_info(info);
    duckdb_vector first_col_vector = duckdb_data_chunk_get_vector(input, 0);
    const uint32_t layer_number = ((uint32_t*)duckdb_vector_get_data(first_col_vector))[0];
    const bool can_be_null = true;
    if ((initialize_local_and_layer(layer_number, num_cols - 1, 1, true, tp_duckdb_state, can_be_null))){
        elog(ERROR, "Error setting up local or layer!");
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);
    if (unlikely(tp_duckdb_state->size_map->find(layer_number) == tp_duckdb_state->size_map->end())) {
        std::vector<uint8_t> *sizes = new std::vector<uint8_t>;
        sizes->push_back(sizeof(uint64_t));
        for (idx_t col_idx = 1; col_idx < num_cols; col_idx++) {
            duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
            void *col_data = (void *)duckdb_vector_get_data(col_vector);
            auto col_type = duckdb_vector_get_column_type(col_vector);
            auto logical_type = reinterpret_cast<duckdb::LogicalType *>(col_type);
            sizes->push_back(logical_type->id() == duckdb::LogicalType::INTEGER ? (sizeof(uint32_t)) : (sizeof(uint64_t)));
            duckdb_destroy_logical_type(&col_type);
        }
        tp_duckdb_state->size_map->insert({layer_number, sizes});
    }
    auto sizes = tp_duckdb_state->size_map->at(layer_number);
    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        void *col_data = (void *)duckdb_vector_get_data(col_vector);
        const uint64_t chunk_size = sizes->at(col_idx - 1) * num_rows;
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, chunk_size);
        main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, chunk_size);
    }

    memset(((bool*)duckdb_vector_get_data(output)), true, sizeof(bool)*num_rows);

    // Append the current size..., yuck.
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(main_layer->rows_layer_number);
    grow_if_full(chunk_size_layer);
    *((uint64_t *)chunk_size_layer->current_row) = num_rows;
    chunk_size_layer->current_row = INCR_BY_BYTES(chunk_size_layer->current_row, sizeof(uint64_t));
    chunk_size_layer->record_count += num_rows;
    chunk_size_layer->num_rows++;

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
            duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
            uint64_t *validity = duckdb_vector_get_validity(col_vector);
            if (validity == NULL) continue;

            bool null_present = false;
            for (idx_t validity_idx = 0; validity_idx < validity_size; validity_idx++){
                if (validity[validity_idx] != (uint64_t)-1){
                    null_present = true;
                    elog(INFO, "Found nulls!");
                    break;
                }
            }
            if (!null_present) continue;
            validity_to_append.push_back(col_idx);
        }
        TRACEPROV_GROW_IF_TRUE(null_layer, ((null_layer->current_row + 2*sizeof(uint64_t)) > null_layer->end_of_memory_zone));
        ((uint64_t*)null_layer->current_row)[0] = current_chunk_idx;
        ((uint64_t*)null_layer->current_row)[1] = validity_to_append.size();
        null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, 2*sizeof(uint64_t));
        for (auto col_idx : validity_to_append)
        {
            duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
            uint64_t *validity = duckdb_vector_get_validity(col_vector);
            // Need to write this column vector's validity.
            // We already know what the size of this vector is (so don't need to write it again)


            // Need to write out the col idx where we say the null.
            // We have validity_size + 1 because also need to write out the column that has null values.
            TRACEPROV_GROW_IF_TRUE(null_layer, ((null_layer->current_row + (validity_size + 1)*sizeof(uint64_t)) > null_layer->end_of_memory_zone));
            ((uint64_t*)null_layer->current_row)[0] = col_idx - 1;
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t));
            memcpy(null_layer->current_row, validity, sizeof(uint64_t)*validity_size);
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t)*validity_size);
        }
    }

}

#define TRACEPROV_DUCKDB_LOG_FUNC_NAME          "traceprov_log_entry_%d"
#define TRACEPROV_DUCKDB_VOLATILE_LOG_FUNC_NAME "traceprov_log_entry_volatile_%d"

duckdb_scalar_function* traceprov_create_log_function(const uint32_t num_args, const bool is_volatile){
    duckdb_scalar_function *funcs = (duckdb_scalar_function *)malloc(sizeof(duckdb_scalar_function) * num_args);
    for (uint32_t idx = 0; idx < num_args; idx++){
        char func_name[256] = {0};
        sprintf(func_name, is_volatile ? TRACEPROV_DUCKDB_VOLATILE_LOG_FUNC_NAME : TRACEPROV_DUCKDB_LOG_FUNC_NAME, idx + 1);
        duckdb_scalar_function func = duckdb_create_scalar_function();
        duckdb_scalar_function_set_name(func, func_name);
        duckdb_logical_type ret_type = duckdb_create_logical_type(DUCKDB_TYPE_BOOLEAN);
        duckdb_scalar_function_set_return_type(func, ret_type);
        duckdb_logical_type layer_number_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_scalar_function_add_parameter(func, layer_number_type);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_ANY);
        for (uint32_t arg_idx = 0; arg_idx < idx + 1; arg_idx++){
            duckdb_scalar_function_add_parameter(func, type);
        }
        duckdb_destroy_logical_type(&layer_number_type);
        duckdb_destroy_logical_type(&type);
        duckdb_destroy_logical_type(&ret_type);
        duckdb_scalar_function_set_function(func, traceprov_log);
        if (is_volatile){
            duckdb_scalar_function_set_volatile(func);
        }
        duckdb_scalar_function_set_special_handling(func);
        auto tp_duckdb_state = new TraceProvDuckDbState;
        tp_duckdb_state->should_hash = traceprov_use_partition_in_log;
        tp_duckdb_state->size_map = new std::unordered_map<TraceProvLayerNumber, std::vector<uint8_t> *>;
        duckdb_scalar_function_set_extra_info(func, tp_duckdb_state, nullptr);
        funcs[idx] = func;
    }
    return funcs;
}

static idx_t traceprov_direct_state_size(const AggregateFunction &function) {
    return sizeof(struct traceprov_agg_context);
}

static void traceprov_direct_state_init(const AggregateFunction &function, data_ptr_t state) {
    auto state_ptr = reinterpret_cast<struct traceprov_agg_context *>(state);
    memset(state_ptr, 0, sizeof(struct traceprov_agg_context));
}

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
    auto bind_ptr = make_uniq<TraceProvAggBind>(extra, layer_number, sizes);
    return bind_ptr;
}

unique_ptr<FunctionData> traceprov_direct_bind(ClientContext &context, AggregateFunction &function, vector<unique_ptr<Expression>> &arguments){
    auto extras = &function.function_info->Cast<TraceProvAggExtra>();
    return shared_bind(context, arguments, extras);
}

static void traceprov_direct_update(Vector inputs[], AggregateInputData &aggr_input_data, idx_t input_count, Vector &states, idx_t count){
    const idx_t num_rows = count;
    if (num_rows == 0) return;
    TraceProvAggExtra *extra;
    idx_t orig_num_cols, num_cols;
    struct traceprov_aggregate_layer *main_layer;
    uint32_t layer_number;
    struct traceprov_agg_context **agg_contexts;

    auto &bind_data = aggr_input_data.bind_data->Cast<TraceProvAggBind>();
    extra = bind_data.extra;

    layer_number = bind_data.layer_number;
    orig_num_cols = input_count;
    num_cols = orig_num_cols;
    
    if (extra->ignore_gn){
        num_cols -= 1;
    }
    const bool can_be_null = true;
    if (initialize_local_and_layer(layer_number, num_cols, 1, true, extra->state, can_be_null)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }

    agg_contexts = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(states);
    main_layer = get_layer(layer_number);

    if (likely(agg_contexts != NULL && !extra->ignore_gn)){
        if (traceprov_use_compact){
            const uint64_t chunk_size = sizeof(uint32_t)*num_rows;
            TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
            for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
                if (agg_contexts[row_idx]->layer_number == 0){
                    agg_contexts[row_idx]->layer_number = layer_number;
                    // Annotate the group with the worker id.
                    agg_contexts[row_idx]->group_cnt = TRACEPROV_SET_WORKER_ID((++main_layer->num_groups), traceprov_current.my_worker_id);
                    agg_contexts[row_idx]->worker_id = traceprov_current.my_worker_id;
                }
                *((uint32_t*)main_layer->current_row) = (uint32_t)agg_contexts[row_idx]->group_cnt;
                main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, sizeof(uint32_t));
                if (unlikely(main_layer->mask == 0)){
                    // Extract out the mask.
                    // During reading, we reapply this mask ;)
                    main_layer->mask = agg_contexts[row_idx]->group_cnt >> 32;
                }
            }
        } else{
            const uint64_t chunk_size = sizeof(uint64_t)*num_rows;
            TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
            for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
                if (agg_contexts[row_idx]->layer_number == 0){
                    agg_contexts[row_idx]->layer_number = layer_number;
                    // Annotate the group with the worker id.
                    agg_contexts[row_idx]->group_cnt = TRACEPROV_SET_WORKER_ID((++main_layer->num_groups), traceprov_current.my_worker_id);
                    agg_contexts[row_idx]->worker_id = traceprov_current.my_worker_id;
                }
                *((uint64_t*)main_layer->current_row) = agg_contexts[row_idx]->group_cnt;
                main_layer->current_row = INCR_BY_BYTES(main_layer->current_row, sizeof(uint64_t));
            }
        }
    }

    auto sizes = bind_data.sizes;
    for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
        inputs[col_idx].Flatten(count);
        void *col_data = FlatVector::GetDataUnsafe<void>(inputs[col_idx]);
        auto compact_chunk_size = (sizes->at(col_idx - 1)*num_rows);
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + compact_chunk_size) > main_layer->end_of_memory_zone));
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
            auto validity = FlatVector::Validity(inputs[col_idx]);
            if (validity.AllValid()) continue;
            validity_to_append.push_back(col_idx);
        }
        TRACEPROV_GROW_IF_TRUE(null_layer, ((null_layer->current_row + 2*sizeof(uint64_t)) > null_layer->end_of_memory_zone));
        ((uint64_t*)null_layer->current_row)[0] = current_chunk_idx;
        ((uint64_t*)null_layer->current_row)[1] = validity_to_append.size();
        null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, 2*sizeof(uint64_t));
        for (auto col_idx : validity_to_append)
        {
            auto validity = FlatVector::Validity(inputs[col_idx]);
            TRACEPROV_GROW_IF_TRUE(null_layer, ((null_layer->current_row + (validity_size + 1)*sizeof(uint64_t)) > null_layer->end_of_memory_zone));
            // In this case, we are using the col_idx directly (rather than -1, because the col_idx = 0 will be the group number)
            ((uint64_t*)null_layer->current_row)[0] = col_idx;
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t));
            memcpy(null_layer->current_row, validity.GetData(), sizeof(uint64_t)*validity_size);
            null_layer->current_row = INCR_BY_BYTES(null_layer->current_row, sizeof(uint64_t)*validity_size);
        }
    }
}

static void traceprov_direct_combine(Vector &state, Vector &combined, AggregateInputData &aggr_input_data, idx_t count){
    state.Flatten(count);
    auto &bind_data = aggr_input_data.bind_data->Cast<TraceProvAggBind>();
    auto extra = bind_data.extra;
    // If we're ignoring group numbers, don't do anything.
    if (extra->ignore_gn){
        return;
    }
    struct traceprov_agg_context **source_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(state);
    struct traceprov_agg_context **target_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(combined);

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

    if(initialize_local_and_layer(combined_layer_number, 2, 1, true, extra->state, 0)){
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
    TRACEPROV_GROW_IF_TRUE(combined_layer, ((combined_layer->current_row + 2*chunk_size) > combined_layer->end_of_memory_zone));

    uint64_t *target_write_ptr = (uint64_t*)combined_layer->current_row;
    uint64_t *source_write_ptr = &(((uint64_t*)combined_layer->current_row)[count]);
    for (idx_t idx = 0; idx < count; idx++){
        if (target_states[idx]->is_combined && source_states[idx]->is_combined)
            elog(ERROR, "Didn't expect both of the states to be combined...");

        if (target_states[idx]->group_cnt == 0 && source_states[idx]->group_cnt > 0){
            if (unlikely(source_states[idx]->is_combined)){
                elog(ERROR, "Expected source to not be combined in this case!");
            }
            memcpy(target_states[idx], source_states[idx], sizeof(struct traceprov_agg_context));
        }

        const struct traceprov_agg_context *source_state = source_states[idx];
        struct traceprov_agg_context *target_state = target_states[idx];

        if (!target_state->is_combined){
            target_state->group_cnt = TRACEPROV_SET_IS_COMBINED(target_state->group_cnt);
            target_state->is_combined = true;
        }

        #if TRACEPROV_COLLECT_STATS_MODE == 1
        ++target_state->combined_count;
        combined_layer->max_combined_times = MAX(combined_layer->max_combined_times, target_state->combined_count);
        #endif
        target_write_ptr[0] = target_state->group_cnt;
        source_write_ptr[0] = source_state->group_cnt;
        target_write_ptr++;
        source_write_ptr++;
    }

    // We're not going to look at this anyways.
    combined_layer->current_row = source_write_ptr;
    main_layer->is_leader_layer = true;
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(combined_layer->rows_layer_number);
    TP_APPEND_CHUNK_SIZE(chunk_size_layer, count);
}

static void traceprov_direct_finalize(Vector &state, AggregateInputData &aggr_input_data, Vector &result, idx_t count, idx_t offset){
    state.Flatten(count);
    struct traceprov_agg_context **source_states = FlatVector::GetDataUnsafe<struct traceprov_agg_context *>(state);
    uint64_t *result_data = FlatVector::GetDataUnsafe<uint64_t>(result);
    for (idx_t i = 0; i < count; i++){
        result_data[offset + i] = source_states[i]->group_cnt;
    }
}

AggregateFunction *traceprov_create_agg_direct_functions(const uint32_t num_args, duckdb::shared_ptr<TraceProvAggExtra> extra){
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
    function->function_info = extra;
    return function;
}

struct TraceProvCreateAggregateFunctionInfo : public CreateFunctionInfo {
	explicit TraceProvCreateAggregateFunctionInfo(AggregateFunction function);
    explicit TraceProvCreateAggregateFunctionInfo(AggregateFunctionSet set);

	AggregateFunctionSet functions;

public:
	unique_ptr<CreateInfo> Copy() const override;
};

TraceProvCreateAggregateFunctionInfo::TraceProvCreateAggregateFunctionInfo(AggregateFunction function)
    : CreateFunctionInfo(CatalogType::AGGREGATE_FUNCTION_ENTRY), functions(function.name) {
	name = function.name;
	functions.AddFunction(std::move(function));
	internal = true;
}

TraceProvCreateAggregateFunctionInfo::TraceProvCreateAggregateFunctionInfo(AggregateFunctionSet set)
    : CreateFunctionInfo(CatalogType::AGGREGATE_FUNCTION_ENTRY), functions(std::move(set)) {
    name = functions.name;
    for (auto &func : functions.functions) {
        func.name = functions.name;
    }
    internal = true;
}

unique_ptr<CreateInfo> TraceProvCreateAggregateFunctionInfo::Copy() const {
    auto result = make_uniq<TraceProvCreateAggregateFunctionInfo>(functions);
    CopyFunctionProperties(*result);
    return std::move(result);
}

void traceprov_create_and_register_agg(const uint32_t max_num_args, duckdb_connection connection){
    auto con = reinterpret_cast<duckdb::Connection *>(connection);
    auto extra = duckdb::make_shared_ptr<TraceProvAggExtra>();
    extra->ignore_gn = false;
    auto tp_duckdb_state = new TraceProvDuckDbState;
    extra->state = tp_duckdb_state;
    tp_duckdb_state->should_hash = traceprov_use_partition_in_agg;
    extra->hasher = std::hash<uint64_t>();
    extra->dynamic_col_cache = nullptr;
    extra->use_part_agg = false;
    // Only in this case both creating entries.
    for (uint32_t idx = 0; idx < TRACEPROV_BUCKET_COUNT; idx++){
        if (tp_duckdb_state->should_hash) {
            extra->slice_vectors[idx] = mmap(
                NULL,
                TRACEPROV_PAGE_SIZE,
                PROT_WRITE,
                TRACEPROV_MMAP_FLAGS,
                0,
                0
            );
        }else{
            extra->slice_vectors[idx] = nullptr;
        }
    }
    for (uint32_t max_arg_limit = 1; max_arg_limit < max_num_args + 1; max_arg_limit++){
        auto agg_func = traceprov_create_agg_direct_functions(max_arg_limit, extra);
        con->BeginTransaction();
        auto &catalog = duckdb::Catalog::GetSystemCatalog(*con->context);
        TraceProvCreateAggregateFunctionInfo sf_info(*agg_func);
        catalog.CreateFunction(*con->context, sf_info);
        con->Commit();
    }
}
#endif
