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


void traceprov_update(duckdb_function_info info, duckdb_data_chunk input, duckdb_aggregate_state *states){
    const idx_t num_rows = duckdb_data_chunk_get_size(input);
    const idx_t num_cols = duckdb_data_chunk_get_column_count(input);
    // PRINT_ON_DEBUG("Number of rows: %d", num_rows);
    // No need to do anything.
    // This needs to be checked here (because we need to look up the first row to get the layer number...)
    if (num_rows == 0) return;

    struct traceprov_agg_context **agg_contexts = (struct traceprov_agg_context **)states;

    duckdb_vector first_col_vector = duckdb_data_chunk_get_vector(input, 0);
    const uint32_t layer_number = ((uint32_t*)duckdb_vector_get_data(first_col_vector))[0];

    if(initialize_local_and_layer(layer_number, num_cols, 1, true)){
        elog(ERROR, "Error setting up local or layer!");
        return;
    }

    for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
        if (agg_contexts[row_idx]->layer_number == 0){
            agg_contexts[row_idx]->layer_number = layer_number;
            agg_contexts[row_idx]->group_cnt = (uint64_t)agg_contexts[row_idx];
        }
    }
    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + sizeof(uint64_t)*TP_STD_VECTOR_SIZE) > main_layer->end_of_memory_zone));
    memcpy(main_layer->current_row, agg_contexts, sizeof(uint64_t)*num_rows);
    main_layer->current_row += sizeof(uint64_t)*TP_STD_VECTOR_SIZE;

    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
        // We need to do simple logging now...
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + sizeof(uint64_t)*TP_STD_VECTOR_SIZE) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, sizeof(uint64_t)*num_rows);
        main_layer->current_row += sizeof(uint64_t)*TP_STD_VECTOR_SIZE;
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

duckdb_aggregate_function *traceprov_create_funcs(const uint32_t num_args){
    duckdb_aggregate_function *funcs = malloc(sizeof(duckdb_aggregate_function) *num_args);
    for (uint32_t idx = 0; idx < num_args; idx++){
        char func_name[256] = {0};
        sprintf(func_name, "traceprov_agg_key_parallel_offset_%d", idx + 1);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
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
    if ((initialize_local_and_layer(layer_number, num_cols - 1, 1, true))){
        elog(ERROR, "Error setting up local or layer!");
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);
    for (idx_t col_idx = 1; col_idx < num_cols; col_idx++){
        duckdb_vector col_vector = duckdb_data_chunk_get_vector(input, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col_vector);
        TRACEPROV_GROW_IF_TRUE(main_layer, ((main_layer->current_row + sizeof(uint64_t)*TP_STD_VECTOR_SIZE) > main_layer->end_of_memory_zone));
        memcpy(main_layer->current_row, col_data, sizeof(uint64_t)*num_rows);
        // We always append by the standard vector size (for alignment, regardless of what the actual size is)
        main_layer->current_row += TP_STD_VECTOR_SIZE*sizeof(uint64_t);
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
#define TRACEPROV_DUCKDB_VOLATILE_LOG_FUNC_NAME "traceprov_volatile_log_entry_%d"

duckdb_scalar_function* traceprov_create_log_function(const uint32_t num_args, const bool is_volatile){
    duckdb_scalar_function *funcs = malloc(sizeof(duckdb_scalar_function) * num_args);
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