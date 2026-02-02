
#include <string>
#include "traceprov_ext_utils.hpp"  
#include "traceprov_infer.hpp"
#include "traceprov_infer_essentials.hpp"

// Duckdb integration for inference.
// Duckdb doesn't technically do anything smart (just runs the query)
// The "leaf" are table scan functions for this mess.

#ifndef STANDARD_VECTOR_SIZE
#define STANDARD_VECTOR_SIZE 2048
#endif


extern "C" {
    #include "duckdb.h"
    #include <stdlib.h>


    struct traceprov_inference_context {
        duckdb_database db;
        duckdb_connection con;
    };

    // This is a bit different from duckdb's global state.
    // We manage the lifecycle of this ourselves.
    typedef struct TraceProvDuckDbGlobalState {
        bool did_initialize;
        std::vector<struct local_context *> *worker_local_contexts;
    } TraceProvDuckDbGlobalState;

    static TraceProvDuckDbGlobalState g_tp_duckdb_state {
        .did_initialize = false,
        .worker_local_contexts = NULL
    };

    typedef struct TraceProvBindData {
        TraceProvRelationArgs rel_args;
        struct traceprov_aggregate_layer *col_layer_info;
        struct traceprov_aggregate_layer *row_layer_info;
        struct traceprov_aggregate_layer *col_null_map_layer_info;
        struct traceprov_aggregate_layer *row_null_map_layer_info;
        uint64_t column_width;
        uint64_t row_width;
    } TraceProvBindData;

    typedef struct TraceProvInitData {
        uint64_t current;
        void *col_layer_ptr;
        void *row_layer_ptr;
        uint64_t number_of_records;
        void *col_null_layer_ptr;
        void *row_null_layer_ptr;
    } TraceProvInitData;

    #define TRACEPROV_MAKE_WORKER_LAYER_KEY(X, Y) ((uint64_t)(((uint64_t)X << 32) | (uint64_t)Y))
    
    void traceprov_duckdb_cleanup(struct traceprov_inference_context *p_ctxt){
        duckdb_disconnect(&p_ctxt->con);
        duckdb_close(&p_ctxt->db);
        g_tp_duckdb_state.did_initialize = false;
        if (g_tp_duckdb_state.worker_local_contexts)
            delete g_tp_duckdb_state.worker_local_contexts;
        g_tp_duckdb_state.worker_local_contexts = nullptr;
    }

    bool traceprov_populate_bind_data(
        const uint32 worker_id,
        const uint32 layer_number,
        TraceProvBindData *bind_data,
        const bool expect_present
    ){
        bind_data->rel_args.worker_id = worker_id; 
        bind_data->rel_args.layer_number = layer_number;

        // Need to figure out the number of columns and everything here.
        if (!g_tp_duckdb_state.did_initialize){
            traceprov_shared_context shared_context;
            if (map_traceprov_shared_context(&shared_context))
                elog(ERROR, "error maping shared context!");

            g_tp_duckdb_state.did_initialize = true;
            g_tp_duckdb_state.worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
        }

        auto current_local_context = g_tp_duckdb_state.worker_local_contexts->at(worker_id - 1);
        auto current_layer = &current_local_context->cached_layers[layer_number - 1];

        // We should always crash here (because the top-level should have detected this case...)
        if (current_layer->layer_number != layer_number){
            if (current_layer->layer_number != 0)
                elog(ERROR, "Invalid state!");
            
                
            if (expect_present){
                elog(ERROR, "Expected the layer number to be filled");
            }
            return false;
        }

        bind_data->col_layer_info = current_layer;
        bind_data->column_width = current_layer->num_pk_records;

        if (unlikely(current_layer->null_map_layer_number != 0)){
            bind_data->col_null_map_layer_info = &current_local_context->cached_layers[current_layer->null_map_layer_number - 1];
            if (bind_data->col_null_map_layer_info->layer_number != current_layer->null_map_layer_number)
                elog(ERROR, "Expected the layer number to be filled");
        }

        if (current_layer->rows_layer_number){
            auto rows_layer = &current_local_context->cached_layers[current_layer->rows_layer_number - 1];
            if (rows_layer->layer_number != current_layer->rows_layer_number)
                elog(ERROR, "Expected the layer number to be filled");
            bind_data->row_layer_info = rows_layer;
            bind_data->row_width = rows_layer->num_pk_records;

            if (unlikely(rows_layer->null_map_layer_number != 0)){
                bind_data->row_null_map_layer_info = &current_local_context->cached_layers[rows_layer->null_map_layer_number - 1];
                if (bind_data->row_null_map_layer_info->layer_number != rows_layer->null_map_layer_number)
                    elog(ERROR, "Expected the layer number to be filled");
            }
        }
        return true;
    }

    void traceprov_populate_init_data(TraceProvBindData *bind_data, TraceProvInitData *init_data_inst){
        init_data_inst->current = 0;
        const auto current_layer = bind_data->col_layer_info;
        map_layer_file(bind_data->rel_args.layer_number, bind_data->rel_args.worker_id, &init_data_inst->col_layer_ptr, current_layer->size);

        if (unlikely(current_layer->null_map)){
            map_layer_file(current_layer->null_map_layer_number, bind_data->rel_args.worker_id, &init_data_inst->col_null_layer_ptr, bind_data->col_null_map_layer_info->size);
        }

        if (bind_data->row_layer_info){
            const auto rows_layer = bind_data->row_layer_info;
            map_layer_file(rows_layer->layer_number, bind_data->rel_args.worker_id, &init_data_inst->row_layer_ptr, rows_layer->size);
            if (unlikely(rows_layer->null_map)){
                map_layer_file(rows_layer->null_map_layer_number, bind_data->rel_args.worker_id, &init_data_inst->row_null_layer_ptr, bind_data->row_null_map_layer_info->size);
            }
        }

        const void *final_ptr = get_final_ptr(init_data_inst->col_layer_ptr, current_layer);
        // Ugh, TODO: This won't be the same when we'll have sorted-by-agg RLE.
        init_data_inst->number_of_records = ((uint64)final_ptr - (uint64)init_data_inst->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);

    }

    void traceprov_duckdb_bind(duckdb_bind_info info) {
        if (duckdb_bind_get_parameter_count(info) != 2){
            elog(ERROR, "Expected 2 params!");
        }

        auto param_1 = duckdb_bind_get_parameter(info, 0);
        const uint32_t current_worker_id = duckdb_get_int64(param_1);
        auto param_2 = duckdb_bind_get_parameter(info, 1);
        const uint32_t layer_number = duckdb_get_int64(param_2);

        duckdb_destroy_value(&param_1);
        duckdb_destroy_value(&param_2);

        auto my_bind_data = (TraceProvBindData *)malloc(sizeof(TraceProvBindData));
        memset(my_bind_data, 0, sizeof(TraceProvBindData));
        traceprov_populate_bind_data(current_worker_id, layer_number, my_bind_data, true);

        for (uint64_t col_count = 0; col_count <  my_bind_data->column_width + my_bind_data->row_width; col_count++){
            const std::string param = std::string("column_") + std::to_string(col_count);
            duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
            duckdb_bind_add_result_column(info, param.c_str(), type);
            duckdb_destroy_logical_type(&type);
        }

        duckdb_bind_set_bind_data(info, my_bind_data, free);
    }

    void traceprov_duckdb_init(duckdb_init_info info){
        auto bind_data = (TraceProvBindData *) duckdb_init_get_bind_data(info);
        auto init_data_inst = (TraceProvInitData *)malloc(sizeof(TraceProvInitData));
        memset(init_data_inst, 0, sizeof(TraceProvInitData));

        traceprov_populate_init_data(bind_data, init_data_inst);
        duckdb_init_set_init_data(info, init_data_inst, free);
    }



    uint64 fillup_pointer(
        const struct traceprov_aggregate_layer *layer,
        duckdb_data_chunk chunk,
        void *source_ptr,
        uint64_t current_pos,
        const uint64 final_num_records,
        const uint64 start_width,
        const uint64 total_width,
        void **final_ptr,
        const void *null_bit_vector
    ){
        
        const uint64_t original_pos = current_pos;
        for (uint64_t i = 0; i < STANDARD_VECTOR_SIZE; i++){
            source_ptr += layer->record_padding;
            if (current_pos >= final_num_records)
                break;
            
            uint64* canonical_ptr = (uint64 *)source_ptr;
            for (uint64 col_idx = 0; col_idx < total_width; col_idx++, canonical_ptr++){
                auto ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, start_width + col_idx));
                ptr[i] = *canonical_ptr;
            }

            source_ptr = (void *)canonical_ptr;
            // Also populate the null-bit vector.
            if (unlikely(layer->null_map != 0)){
                const uint64_t null_bit_map = layer->null_map;
                const uint64_t null_bit_set = ((uint64_t*)null_bit_vector)[current_pos];
                for (uint64_t col_idx = 0; col_idx < total_width; col_idx++){
                    duckdb_vector col_vector = duckdb_data_chunk_get_vector(chunk, start_width + col_idx);
                    duckdb_vector_ensure_validity_writable(col_vector);
                    if ((null_bit_map & (((uint64_t)1) << col_idx)) != 0){
                        if (null_bit_set & (((uint64_t)1) << col_idx)){
                            auto validity = duckdb_vector_get_validity(col_vector);
                            duckdb_validity_set_row_invalid(validity, i);
                        }
                    }
                }
            }
            current_pos++;
        }

        duckdb_data_chunk_set_size(chunk, current_pos - original_pos);
        *final_ptr = source_ptr;
        return current_pos;
    }

    void traceprov_duckdb_func(duckdb_function_info info, duckdb_data_chunk output){

        auto bind_data = (TraceProvBindData *)duckdb_function_get_bind_data(info);
        auto init_data = (TraceProvInitData *)duckdb_function_get_init_data(info);
        auto final_state = fillup_pointer(bind_data->col_layer_info, output, init_data->col_layer_ptr, init_data->current, init_data->number_of_records, 0, bind_data->column_width, &init_data->col_layer_ptr, init_data->col_null_layer_ptr);
        if (bind_data->row_layer_info){
            fillup_pointer(bind_data->row_layer_info, output, init_data->row_layer_ptr, init_data->current, init_data->number_of_records, bind_data->column_width, bind_data->row_width, &init_data->row_layer_ptr, init_data->row_null_layer_ptr);
        }
        init_data->current = final_state;
    }

    #define PG_DUCKDB_EXIT_ON_ERROR(state) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d", __FILE__,  __LINE__); \
        } \
    }

    #define PG_DUCKDB_EXIT_ON_ERROR_MSG(state, msg) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__,  __LINE__, msg); \
        } \
    }

    #define PG_DUCKDB_EXIT_ON_ERROR_RESULT(state, result) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__,  __LINE__, duckdb_result_error(&result)); \
        } \
    }

    #define PG_DUCKDB_RUN_SHORT_QUERY(con, query, msg) { \
        duckdb_result result; \
        elog(INFO, "QUERY: %s", query); \
        duckdb_state state = duckdb_query(con, query, &result); \
        PG_DUCKDB_EXIT_ON_ERROR_RESULT(state, result); \
        duckdb_destroy_result(&result); \
        elog(INFO, "Reached: %s correctly", msg); \
    } \


    void traceprov_window_func(duckdb_function_info info, duckdb_data_chunk input, duckdb_vector output){
        TraceProvWindowFuncExtra *window_extra = (TraceProvWindowFuncExtra *)duckdb_scalar_function_get_extra_info(info);
        if (unlikely(window_extra == NULL)){
            elog(ERROR, "Expected window func extra to be set!");
        }
        duckdb_vector worker_id_vector = duckdb_data_chunk_get_vector(input, 0);
        uint32_t *worker_id_data = (uint32_t *)duckdb_vector_get_data(worker_id_vector);
        
        duckdb_vector layer_number_vector = duckdb_data_chunk_get_vector(input, 1);
        uint32_t *layer_number_data = (uint32_t *)duckdb_vector_get_data(layer_number_vector);
        
        duckdb_vector frame_start_vector = duckdb_data_chunk_get_vector(input, 2);
        uint64_t *frame_start_data = (uint64_t *)duckdb_vector_get_data(frame_start_vector);

        duckdb_vector frame_end_vector = duckdb_data_chunk_get_vector(input, 3);
        uint64_t *frame_end_data = (uint64_t *)duckdb_vector_get_data(frame_end_vector);

        const idx_t row_count = duckdb_data_chunk_get_size(input);

        idx_t expected_size = 0;
        for (idx_t row_idx = 0; row_idx < row_count; row_idx++){
            if (unlikely(frame_end_data[row_idx] < frame_start_data[row_idx])){
                elog(ERROR, "Expected end to never be less than start!");
            }
            // Since a frame always includes the current row, also need to add 1.
            // This could be an upper bound in the cases where the worker is simply not present..
            expected_size += (frame_end_data[row_idx] - frame_start_data[row_idx]) + 1;
        }
            
        if(duckdb_list_vector_reserve(output, expected_size) == DuckDBError){
            elog(ERROR, "Error reserving!");
        }
        
        if(duckdb_list_vector_set_size(output, expected_size) == DuckDBError){
            elog(ERROR, "Error setting size!");
        }

        auto entries = (duckdb_list_entry *)duckdb_vector_get_data(output);

        duckdb_vector child_structs = duckdb_list_vector_get_child(output);
        uint64_t generic_idx = 0;
        for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
            const uint32_t worker_id = worker_id_data[row_idx];
            const uint32_t layer_number = layer_number_data[row_idx];
            TraceProvWindowPack *window_pack = window_extra->key_bind_map->at(TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number));

            const uint64_t frame_start = frame_start_data[row_idx];
            const uint64_t frame_end = frame_end_data[row_idx];
            const uint64_t generic_start_idx = generic_idx;
            for (uint64_t row_to_return = frame_start; row_to_return < (frame_end + 1); row_to_return++, generic_idx++){
                const int64_t logged_row_idx = row_to_return - 1;
                if (unlikely(logged_row_idx < 0))
                    elog(ERROR, "any idx can never be < 0!");
                for (uint64_t col_idx = 0; col_idx < window_extra->num_cols; col_idx++){
                    duckdb_vector member_data = duckdb_struct_vector_get_child(child_structs, col_idx);
                    uint64_t *child_data = (uint64_t *)duckdb_vector_get_data(member_data);
                    traceprov_aggregate_layer *curr_layer = window_pack->bind_data->row_layer_info;;
                    void *data_ptr = window_pack->init_data->row_layer_ptr;
                    child_data[generic_idx] = ((uint64_t*)(&(((uint8_t*)data_ptr)[(TRACEPROV_GET_RECORD_SIZE(curr_layer)*logged_row_idx) + (curr_layer->record_padding)])))[col_idx];
                }
            }
            const uint64_t generic_end_idx = generic_idx;
            // TODO: get rid of extra vars.
            // Helpful for readability ig.
            entries[row_idx].offset = generic_start_idx;
            entries[row_idx].length = generic_end_idx - generic_start_idx;
        }
    }

    static duckdb_scalar_function traceprov_create_read_window_func(
        const uint64_t num_args,
        const uint32_t worker_count,
        std::vector<uint32_t> *expected_layers
    ){
        duckdb_scalar_function func = duckdb_create_scalar_function();
        std::string *func_name = new std::string(("traceprov_read_window_" + std::to_string(num_args)).c_str());
        duckdb_scalar_function_set_name(func, func_name->c_str());
        duckdb_logical_type basic_arg_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_logical_type arg_type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
        // Worker id.
        duckdb_scalar_function_add_parameter(func, basic_arg_type);
        // Layer id.
        duckdb_scalar_function_add_parameter(func, basic_arg_type);
        // Frame start
        duckdb_scalar_function_add_parameter(func, arg_type);
        // Frame end.
        duckdb_scalar_function_add_parameter(func, arg_type);

        duckdb_logical_type *struct_member_types = (duckdb_logical_type *)malloc(sizeof(duckdb_logical_type)*num_args);
        const char **member_names = (const char **)malloc(sizeof(char *)*num_args);
        for (uint64_t col_idx = 0; col_idx < num_args; col_idx++){
            std::string *column_str = new std::string((std::string("column_") + std::to_string(col_idx)).c_str());
            member_names[col_idx] = column_str->c_str();
            struct_member_types[col_idx] = arg_type;
        }
        duckdb_logical_type struct_type = duckdb_create_struct_type(struct_member_types, member_names, num_args);
        duckdb_logical_type list_type = duckdb_create_list_type(struct_type);
        duckdb_scalar_function_set_return_type(func, list_type);
        duckdb_destroy_logical_type(&list_type);
        duckdb_scalar_function_set_function(func, traceprov_window_func);
        duckdb_destroy_logical_type(&arg_type);

        auto window_extra = new TraceProvWindowFuncExtra;
        window_extra->key_bind_map = new std::unordered_map<uint64_t, TraceProvWindowPack *>;

        for (uint32_t worker_id = 1; worker_id < worker_count + 1; worker_id++){
            for (auto layer_number: *expected_layers){
                const uint64_t worker_layer_key = TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number);
                if (unlikely(window_extra->key_bind_map->find(worker_layer_key) != window_extra->key_bind_map->end())){
                    elog(ERROR, "Expected the key to not be present!");
                }
                TraceProvBindData *bind_data = new TraceProvBindData;
                TraceProvInitData *init_data = new TraceProvInitData;
                memset(bind_data, 0, sizeof(TraceProvBindData));
                memset(init_data, 0, sizeof(TraceProvInitData));
                TraceProvWindowPack *window_pack = nullptr;
                // In the case where we dont' find it, it'll just be null.
                // This simplifies checking for it later, or at least makes the code more readable.
                if(traceprov_populate_bind_data(worker_id, layer_number, bind_data, false)){
                    window_pack = new TraceProvWindowPack;
                    window_pack->bind_data = bind_data;
                    window_pack->init_data = init_data;
                    traceprov_populate_init_data(bind_data, init_data);
                }
                window_extra->key_bind_map->insert({worker_layer_key, window_pack});
            }
        }
        window_extra->num_cols = num_args;
        // We don't care about the extra being "regenerated".
        // This is becuase the input layers are always disjoint.
        // So, the same worker+layer will never occur again, across the calls.
        duckdb_scalar_function_set_extra_info(func, window_extra, nullptr);
        return func;
    }

    static duckdb_table_function setup_func(){
        auto function = duckdb_create_table_function();
        duckdb_table_function_set_name(function, "traceprov_read_worker_layer");
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_table_function_add_parameter(function, type);
        duckdb_table_function_add_parameter(function, type);
        duckdb_destroy_logical_type(&type);

        duckdb_table_function_set_bind(function, traceprov_duckdb_bind);
        duckdb_table_function_set_init(function, traceprov_duckdb_init);
        duckdb_table_function_set_function(function, traceprov_duckdb_func);
        return function;
    }

    static void populate_traceprov_data(
        TraceProvData *traceprov_data, 
        duckdb_data_chunk *chunk
    ){
        const uint64 column_count = duckdb_data_chunk_get_column_count(*chunk);
        const uint64 row_count = duckdb_data_chunk_get_size(*chunk);
        // Unlikely because it'll happen just once, for the first chunk.
        if (unlikely(column_count != traceprov_data->size())){
            if (traceprov_data->size() != 0){
                elog(ERROR, "Attempting to set different number of column entries");
            }
            for (uint64 col_idx = 0; col_idx < column_count; col_idx++){
                auto column_data = new TraceProvColumnData;
                column_data->data = new std::vector<uint64>();
                column_data->validity = new std::vector<bool>();
                traceprov_data->push_back(column_data);
            }
        }
        for (uint64 col_idx = 0; col_idx < column_count; col_idx++){
            duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
            uint64 *col_data = (uint64 *)duckdb_vector_get_data(col);
            uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
            auto current_column_data = traceprov_data->at(col_idx)->data;
            auto current_column_data_validity = traceprov_data->at(col_idx)->validity;
            current_column_data->reserve(row_count + current_column_data->size() + 50);
            current_column_data_validity->reserve(row_count + current_column_data_validity->size() + 50);
            for (uint64 row_idx = 0; row_idx < row_count; row_idx++){
                if (col_validity != NULL){ 
                    current_column_data_validity->push_back(duckdb_validity_row_is_valid(col_validity, row_idx));
                } else{
                    current_column_data_validity->push_back(true);
                }
                current_column_data->push_back(col_data[row_idx]);
            }
        }
    }

    void traceprov_duckdb_setup_context(
        struct traceprov_inference_context **p_ctxt,
        void (**p_cleanup)(struct traceprov_inference_context *),
        TraceProvInferSetupExtra *setup_extra
    ){
        struct traceprov_inference_context *context = (struct traceprov_inference_context *) malloc(sizeof(struct traceprov_inference_context));
        duckdb_database db;
        duckdb_connection con;
        char *error_msg;
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(":memory:", &db, nullptr, &error_msg), error_msg);
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));
        context->db = db;
        context->con = con;
        *p_ctxt = context;
        
        for (auto entry: *setup_extra->size_layer_map){
            duckdb_scalar_function read_window_func = traceprov_create_read_window_func(entry.first, setup_extra->worker_count, entry.second);
            PG_DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, read_window_func));
        }
        
        auto function = setup_func();
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
        PG_DUCKDB_RUN_SHORT_QUERY(con, "SET threads=1;", "setting threads");
        *p_cleanup = traceprov_duckdb_cleanup;
    }

    // guts of all the inference.
    TraceProvData *traceprov_perform_duckdb_inference(
        const char *generated_sql,
        struct traceprov_inference_context *context
    ){
        duckdb_connection con = context->con;
        duckdb_prepared_statement stmt;
        duckdb_result final_result;
        std::string final_sql_str = std::string(generated_sql);
        // final_sql_str = "copy (" + final_sql_str + ") to '/tmp/temp.csv'";
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, final_sql_str.c_str(), &stmt), duckdb_prepare_error(stmt));
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));

        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        auto traceprov_data = new TraceProvData;
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++){
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            populate_traceprov_data(traceprov_data, &data_chunk);
            duckdb_destroy_data_chunk(&data_chunk);
        }

        // PG_DUCKDB_RUN_SHORT_QUERY(con, generated_sql, "inference query");
        return traceprov_data;
    }
}