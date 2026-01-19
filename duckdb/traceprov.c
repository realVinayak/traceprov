#include "traceprov.h"
#include "utils.h"
#include "duckdb.h"
#include <string.h>
#include "file_utils.h"

struct current_context traceprov_current = {.my_worker_id = 0,
                                            .traceprov_shared_context_fd = -1,
                                            .shared_context = NULL,
                                            .local_context = NULL,
                                            .maximum_local_layer_used = 0};

static inline struct traceprov_aggregate_layer *get_layer(const uint32_t layer_number){
    if (layer_number == 0){
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


void traceprov_update(duckdb_function_info info, duckdb_data_chunk input, duckdb_aggregate_state *states){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);
    const idx_t num_cols = duckdb_data_chunk_get_column_count(input);
    // PRINT_ON_DEBUG("Number of rows: %d", num_rows);
    // No need to do anything.
    // This needs to be checked here (because we need to look up the first row to get the layer number...)
    if (num_rows == 0) return;

    struct traceprov_agg_context **agg_contexts = (struct traceprov_agg_context **)states;
    // Can technically be merged with row_idx but this way it is more likely to be vectorized.
    struct traceprov_aggregate_layer *infered_main_layer = NULL;

    duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, 0);
    uint32_t *col_data = (uint32_t *)duckdb_vector_get_data(col_vector);
    for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
        if (agg_contexts[row_idx]->layer_number == 0){
            // In this case, need to initialized.
            // We can't assume that we'll always be called in order (maybe we can, maybe we cannot)
            // That is, it may call us with (1, blah) and then (2, blah) will be the second state.
            const uint64_t incoming_layer_number = col_data[row_idx];
            if(initialize_local_and_layer(incoming_layer_number, 1, num_cols - 1, true)){
                elog(ERROR, "Error setting up local or layer!");
                return;
            }
            agg_contexts[row_idx]->layer_number = incoming_layer_number;
            // We also take this opportunity to increment the group number.
            struct traceprov_aggregate_layer *main_layer = get_layer(incoming_layer_number);
            agg_contexts[row_idx]->group_cnt = ++main_layer->num_groups;
        }
        // Also log the group number here.
        // This way, the other columns just become simple appends AND we won't touch the main layer file
        // wohoo, nice for LRU caches. This needs to be out of the previous condition, because we need
        // to log it for all the rows
        // _technically_ it is possible that we get called for different layer numbers....
        const struct traceprov_agg_context *current_agg_context = agg_contexts[row_idx];
        struct traceprov_aggregate_layer *main_layer = get_layer(current_agg_context->layer_number);
        grow_if_full(main_layer);
        *((uint64_t*)main_layer->current_row) = (current_agg_context->group_cnt);
        main_layer->current_row += sizeof(uint64_t);
        infered_main_layer = main_layer;
    }

    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
        // We need to do simple logging now...
        struct traceprov_aggregate_layer *rows_layer = get_layer(infered_main_layer->rows_layer_number);
        TRACEPROV_GROW_IF_TRUE(rows_layer, ((rows_layer->current_row + sizeof(uint64_t)*num_rows) > rows_layer->end_of_memory_zone));
        memcpy(rows_layer->current_row, col_data, sizeof(uint64_t)*num_rows);
        rows_layer->current_row += sizeof(uint64_t)*num_rows;
    }
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
        }
        ref_group_number = target_state->group_cnt;

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

duckdb_aggregate_function *traceprov_create_funcs(const uint32_t num_args){
    duckdb_aggregate_function *funcs = malloc(sizeof(duckdb_aggregate_function) *num_args);
    for (uint32_t idx = 0; idx < num_args; idx++){
        char func_name[256] = {0};
        sprintf(func_name, "traceprov_agg_key_parallel_offset_%d", idx + 1);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
        duckdb_aggregate_function func = (duckdb_aggregate_function) duckdb_create_aggregate_function();
        PRINT_ON_DEBUG("name: %s", func_name);
        duckdb_aggregate_function_set_name(func, func_name);
        duckdb_logical_type first_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_aggregate_function_add_parameter(func, first_type);
        const uint32_t total_arg_count = idx + 1;
        for (uint32_t arg_idx = 0; arg_idx < total_arg_count; arg_idx++){
            duckdb_aggregate_function_add_parameter(func, type);
        }
        duckdb_aggregate_function_set_return_type(func, type);
        duckdb_destroy_logical_type(&type);
        duckdb_destroy_logical_type(&first_type);
        duckdb_aggregate_function_set_functions(func, traceprov_get_state_size, traceprov_initialize, traceprov_update, traceprov_combine, traceprov_finalize);
        funcs[idx] = func;
    }
    return funcs;
}

void traceprov_reinit_state(duckdb_function_info, duckdb_data_chunk input, duckdb_vector output){
    traceprov_current.my_worker_id = 0;
    traceprov_current.my_worker_id = 0;
    traceprov_current.traceprov_shared_context_fd  = -1;
    traceprov_current.shared_context = NULL;
    traceprov_current.local_context = NULL;
    traceprov_current.maximum_local_layer_used = 0;
    char *traceprov_data_dir = malloc(sizeof(char)*512);
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
    if ((initialize_local_and_layer(layer_number, num_cols - 1, 0, true))){
        elog(ERROR, "Error setting up local or layer!");
    }
    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
    }
}

duckdb_scalar_function traceprov_create_log_function(const uint32_t num_args){

}