
#include "traceprov.hpp"
#include "utils.hpp"
#include <unistd.h>
#include "duckdb.hpp"
#include <string.h>
#include "file_utils.hpp"
#include <sys/mman.h>

struct current_context traceprov_current = {.my_worker_id = 0,
                                            .traceprov_shared_context_fd = -1,
                                            .shared_context = NULL,
                                            .local_context = NULL,
                                            .maximum_local_layer_used = 0};

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
    const bool set_current_row
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
        set_current_row
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

idx_t traceprov_get_state_size(duckdb_function_info info){
    return sizeof(struct traceprov_agg_context);
}

void traceprov_initialize(duckdb_function_info info, duckdb_aggregate_state state){
    // TODO: Try pulling some of the stuff out from the update to here... (like layer number, by
    // adding a bind function....)
    struct traceprov_agg_context *agg_context = (struct traceprov_agg_context *)state;
    memset(agg_context, 0, sizeof(struct traceprov_agg_context));
}


static inline int round_up(const int number){
    return number == 1 ? 1 : (1 << (64 - __builtin_clzl(number - 1)));
}

void traceprov_update(duckdb_function_info info, duckdb_data_chunk input, duckdb_aggregate_state *states){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);

    PRINT_ON_DEBUG("Number of rows: %d", num_rows);
    // No need to do anything.
    // This needs to be checked here (because we need to look up the first row to get the layer number...)
    if (num_rows == 0) return;

    TraceProvAggExtra *extra = (TraceProvAggExtra *) duckdb_aggregate_function_get_extra_info(info);
    const idx_t orig_num_cols = duckdb_data_chunk_get_column_count(input);
    idx_t num_cols = orig_num_cols;
    // If ignoring group numbers, don't need to log it (or count it as part of width)
    if (extra->ignore_gn)
        num_cols -= 1;

    struct traceprov_agg_context **agg_contexts = (struct traceprov_agg_context **)states;

    duckdb_vector first_col_vector = duckdb_data_chunk_get_vector(input, 0);
    const uint32_t layer_number = ((uint32_t*)duckdb_vector_get_data(first_col_vector))[0];

    if(initialize_local_and_layer(layer_number, num_cols, 1, true)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    const uint64_t chunk_size = sizeof(uint64_t)*num_rows;
    if (likely(agg_contexts != NULL && !extra->ignore_gn)){
        for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
            if (agg_contexts[row_idx]->layer_number == 0){
                agg_contexts[row_idx]->layer_number = layer_number;
                agg_contexts[row_idx]->group_cnt = (uint64_t)agg_contexts[row_idx];
            }
        }
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, agg_contexts, chunk_size);
        main_layer->current_row += chunk_size;
    }

    const idx_t true_column_count = duckdb_data_chunk_get_column_count(input);
    for (idx_t col_idx = 1; col_idx < orig_num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, chunk_size);
        main_layer->current_row += chunk_size;
    }

    // Append the current size..., yuck.
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(main_layer->rows_layer_number);
    grow_if_full(chunk_size_layer);
    *((uint64_t *)chunk_size_layer->current_row) = num_rows;
    chunk_size_layer->current_row += sizeof(uint64_t);
    chunk_size_layer->num_rows++;
}

void traceprov_combine(
    duckdb_function_info info,
    duckdb_aggregate_state *source_p,
    duckdb_aggregate_state *target_p,
    idx_t count
){
    struct traceprov_agg_context **source_states = (struct traceprov_agg_context **)source_p;
    struct traceprov_agg_context **target_states = (struct traceprov_agg_context **)target_p;
    for (idx_t idx = 0; idx < count; idx++){
        if (target_states[idx]->is_combined && source_states[idx]->is_combined)
            elog(ERROR, "Didn't expect both of the states to be combined...");
        if (target_states[idx]->group_cnt == 0 && source_states[idx]->group_cnt > 0)
            memcpy(target_states[idx], source_states[idx], sizeof(struct traceprov_agg_context));
        // Makes sense to just do this one inline (in a row-based fashion for now...)
        uint64_t ref_group_number = 0;
        // not touching this
        const struct traceprov_agg_context *source_state = source_states[idx];
        struct traceprov_agg_context *target_state = target_states[idx];
        const uint32_t layer_number = target_state->layer_number;
        struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

        uint32_t combined_layer_number = 0;
        // it is entirely possible that we see not combined aggs for the same layer.
        // For those cases, need to check if we set the combined in the main layer.
        if (((combined_layer_number = main_layer->combined_aggregate_layer_number) == 0)){
            main_layer->combined_aggregate_layer_number = ++traceprov_current.maximum_local_layer_used;
            combined_layer_number =  main_layer->combined_aggregate_layer_number;
        }
        if (!target_state->is_combined){
            // ugh. fine for now...
            ref_group_number = ++main_layer->num_groups;
        }else{
            ref_group_number = target_state->group_cnt;
        }

        if(initialize_local_and_layer(combined_layer_number, 2, 0, true)){
            elog(ERROR, "Error setting up local or layer!");
            return;
        }
        struct traceprov_aggregate_layer *combined_layer = get_layer(combined_layer_number);
        TRACEPROV_GROW_IF_TRUE(
            combined_layer,
            ((combined_layer->current_row == combined_layer->end_of_memory_zone) 
            || ((combined_layer->current_row == (combined_layer->end_of_memory_zone - (TRACEPROV_GET_RECORD_SIZE(combined_layer)))) && !target_state->is_combined))
        );

        if (!target_state->is_combined){
            TRACEPROV_INCREMENT_BY_PADDING(combined_layer);
            ((uint64_t *)combined_layer->current_row)[0] = ref_group_number;
            ((uint64_t *)combined_layer->current_row)[1] = target_state->group_cnt;
            combined_layer->current_row += (TRACEPROV_GET_RECORD_SIZE(combined_layer));
            target_state->is_combined = true;
        }
        TRACEPROV_INCREMENT_BY_PADDING(combined_layer);
        ((uint64_t *)combined_layer->current_row)[0] = ref_group_number;
        ((uint64_t *)combined_layer->current_row)[1] = source_state->group_cnt;
        target_state->group_cnt = ref_group_number;
    }
}

void traceprov_finalize(duckdb_function_info info, duckdb_aggregate_state *source_p, duckdb_vector result, idx_t count, idx_t offset){
    struct traceprov_agg_context **source_states = (struct traceprov_agg_context **)source_p;
    uint64_t *result_data = (uint64_t *)duckdb_vector_get_data(result);
    for (idx_t i = 0; i < count; i++){
        result_data[offset + i] = source_states[i]->group_cnt;
    }
}

// Taken from duckdb src.
duckdb::AggregateFunction *GetCAggregateFunction(duckdb_aggregate_function function) {
    return reinterpret_cast<duckdb::AggregateFunction *>(function);
}

duckdb_aggregate_function *traceprov_create_funcs(
    const uint32_t num_args,
    const bool is_window,
    const bool ignore_group_number
){
    duckdb_aggregate_function *funcs = (duckdb_aggregate_function *)malloc(sizeof(duckdb_aggregate_function) *num_args);
    for (uint32_t idx = 0; idx < num_args; idx++){
        std::string func_name = "traceprov_agg_key_parallel_offset_";
        if (is_window){
            func_name += "window_";
        }
        if (ignore_group_number){
            func_name += "ignore_gn_";
        }
        func_name += std::to_string(idx + 1);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
        duckdb_aggregate_function func = (duckdb_aggregate_function) duckdb_create_aggregate_function();
        PRINT_ON_DEBUG("name: %s", func_name.c_str());
        duckdb_aggregate_function_set_name(func, (new std::string(func_name))->c_str());
        duckdb_logical_type first_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_aggregate_function_add_parameter(func, first_type);
        const uint32_t total_arg_count = idx + 1;
        for (uint32_t arg_idx = 0; arg_idx < total_arg_count; arg_idx++){
            duckdb_aggregate_function_add_parameter(func, type);
        }
        duckdb_aggregate_function_set_return_type(func, type);
        auto extra = new TraceProvAggExtra;
        extra->ignore_gn = ignore_group_number;
        duckdb_aggregate_function_set_extra_info(func, extra, nullptr);
        duckdb_destroy_logical_type(&type);
        duckdb_destroy_logical_type(&first_type);
        duckdb_aggregate_function_set_functions(func, traceprov_get_state_size, traceprov_initialize, traceprov_update, traceprov_combine, traceprov_finalize);
        auto base = GetCAggregateFunction(func);
        // IDK why the C-API requires the combine.
        // TODO: Experiment with disabling this.
        // base.combine = nullptr;
        funcs[idx] = func;
    }
    return funcs;
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
        traceprov_update(NULL, reinterpret_cast<duckdb_data_chunk>(&page), NULL);
        last_chunk_idx = scan->chunk_index;
    }
}

duckdb_aggregate_function *traceprov_create_window_funcs(const uint32_t num_args){
    duckdb_aggregate_function *base_functions = traceprov_create_funcs(num_args, true);
    for (uint32_t idx = 0; idx < num_args; idx++){
        auto base_agg_function = GetCAggregateFunction(base_functions[idx]);
        // We're basically creating the window variant of this func.
        // need to disable everything that isn't window.
        base_agg_function->combine = nullptr;
        base_agg_function->finalize = nullptr;
        base_agg_function->simple_update = nullptr;
        base_agg_function->window = traceprov_window;
    }
    return base_functions;
}


void traceprov_reinit_state(duckdb_function_info, duckdb_data_chunk input, duckdb_vector output){
    #if TRACEPROV_USE_MMEM_PAGE
    if (traceprov_current.local_context != NULL){
        // cleanup mem stuff (unmapping)
        for (uint32_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            const struct traceprov_aggregate_layer *agg_layer = &traceprov_current.local_context->cached_layers[layer_idx];
            if (agg_layer->layer_number == 0 || agg_layer->page_mapping == NULL) continue;
            for (uint32_t mapping_id = 0; mapping_id < agg_layer->page_mapping_size; mapping_id++){
                void *page_ptr = agg_layer->page_mapping[mapping_id];
                const uint64_t page_size = mapping_id == 0 ?  TRACEPROV_PAGE_SIZE : (TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE);
                if(munmap(page_ptr, page_size)){
                    PRINT_ON_DEBUG("Got error stage when unmapping!");
                    elog(ERROR, "Got error stage when unmapping!");
                }
            }
            free(agg_layer->page_mapping);
        }
    }
    #endif

    traceprov_current.my_worker_id = 0;
    traceprov_current.my_worker_id = 0;
    traceprov_current.traceprov_shared_context_fd  = -1;
    traceprov_current.shared_context = NULL;
    traceprov_current.local_context = NULL;
    traceprov_current.maximum_local_layer_used = 0;
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

void traceprov_log(duckdb_function_info, duckdb_data_chunk input, duckdb_vector output){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);
    const idx_t num_cols = duckdb_data_chunk_get_column_count(input);
    if (num_rows == 0) return;
    duckdb_vector first_col_vector = duckdb_data_chunk_get_vector(input, 0);
    const uint32_t layer_number = ((uint32_t*)duckdb_vector_get_data(first_col_vector))[0];
    if ((initialize_local_and_layer(layer_number, num_cols - 1, 1, true))){
        elog(ERROR, "Error setting up local or layer!");
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);
    const uint64_t chunk_size = sizeof(uint64_t)*num_rows;
    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + chunk_size) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, sizeof(uint64_t)*num_rows);
        main_layer->current_row += chunk_size;
    }

    memset(((bool*)duckdb_vector_get_data(output)), true, sizeof(bool)*num_rows);

    // Append the current size..., yuck.
    struct traceprov_aggregate_layer *chunk_size_layer = get_layer(main_layer->rows_layer_number);
    grow_if_full(chunk_size_layer);
    *((uint64_t *)chunk_size_layer->current_row) = num_rows;
    chunk_size_layer->current_row += sizeof(uint64_t);
    chunk_size_layer->num_rows++;
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
        duckdb_logical_type first_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_scalar_function_add_parameter(func, first_type);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
        for (uint32_t arg_idx = 0; arg_idx < idx + 1; arg_idx++){
            duckdb_scalar_function_add_parameter(func, type);
        }
        duckdb_destroy_logical_type(&first_type);
        duckdb_destroy_logical_type(&type);
        duckdb_destroy_logical_type(&ret_type);
        duckdb_scalar_function_set_function(func, traceprov_log);
        if (is_volatile){
            duckdb_scalar_function_set_volatile(func);
        }
        funcs[idx] = func;
    }
    return funcs;
}
#endif