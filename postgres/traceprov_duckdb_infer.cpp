
#include <string>
#include "traceprov_ext_utils.hpp"  
#include "traceprov_infer.hpp"

// Duckdb integration for inference.
// Duckdb doesn't technically do anything smart (just runs the query)
// The "leaf" are table scan functions for this mess.

#ifndef STANDARD_VECTOR_SIZE
#define STANDARD_VECTOR_SIZE 2048
#endif


extern "C" {
    #include "duckdb.h"
    #include <stdlib.h>
    #include "traceprov_infer_essentials.h"

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

    void reinit_traceprov_infer_state(){
        g_tp_duckdb_state.did_initialize = false;
        g_tp_duckdb_state.worker_local_contexts = NULL;
    }

    typedef struct TraceProvBindData {
        TraceProvRelationArgs rel_args;
        struct traceprov_aggregate_layer *col_layer_info;
        struct traceprov_aggregate_layer *row_layer_info;
        void *col_layer_ptr;
        void *row_layer_ptr;
        uint64_t number_of_records;
        uint64_t column_width;
        uint64_t row_width;
    } TraceProvBindData;

    typedef struct TraceProvInitData {
        uint64_t current;
    } TraceProvInitData;

    void traceprov_duckdb_bind(duckdb_bind_info info) {
        if (duckdb_bind_get_parameter_count(info) != 2){
            elog(ERROR, "Expected 2 params!");
        }

        auto my_bind_data = (TraceProvBindData *)malloc(sizeof(TraceProvBindData));
        memset(my_bind_data, 0, sizeof(TraceProvBindData));
        auto param_1 = duckdb_bind_get_parameter(info, 0);
        const uint64_t current_worker_id = duckdb_get_int64(param_1);
        auto param_2 = duckdb_bind_get_parameter(info, 1);
        const uint64_t layer_number = duckdb_get_int64(param_2);

        duckdb_destroy_value(&param_1);
        duckdb_destroy_value(&param_2);

        my_bind_data->rel_args.worker_id = current_worker_id; 
        my_bind_data->rel_args.layer_number = layer_number;

        // Need to figure out the number of columns and everything here.
        if (!g_tp_duckdb_state.did_initialize){
            traceprov_shared_context shared_context;
            if (map_traceprov_shared_context(&shared_context))
                elog(ERROR, "error maping shared context!");

            g_tp_duckdb_state.did_initialize = false;
            g_tp_duckdb_state.worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
        }

        auto current_local_context =  g_tp_duckdb_state.worker_local_contexts->at(current_worker_id - 1);
        auto current_layer = &current_local_context->cached_layers[layer_number - 1];

        // We should always crash here (because the top-level should have detected this case...)
        if (current_layer->layer_number != layer_number)
            elog(ERROR, "Expected the layer number to be filled");

        my_bind_data->col_layer_info = current_layer;
        map_layer_file(current_layer->layer_number, current_worker_id, &my_bind_data->col_layer_ptr, current_layer->size);
        uint64 column_count = current_layer->num_pk_records;
        my_bind_data->column_width = column_count;
        if (current_layer->rows_layer_number){
            auto rows_layer = &current_local_context->cached_layers[current_layer->rows_layer_number - 1];
            if (rows_layer->layer_number != current_layer->rows_layer_number)
                elog(ERROR, "Expected the layer number to be filled");
            column_count += rows_layer->num_pk_records;
            my_bind_data->row_layer_info = rows_layer;
            my_bind_data->row_width = rows_layer->num_pk_records;
            map_layer_file(rows_layer->layer_number, current_worker_id, &my_bind_data->row_layer_ptr, rows_layer->size);
        }

        for (uint64_t col_count = 0; col_count < column_count; col_count++){
            const std::string param = std::string("column_") + std::to_string(col_count);
            duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
            duckdb_bind_add_result_column(info, param.c_str(), type);
            duckdb_destroy_logical_type(&type);
        }

        const void *final_ptr = get_final_ptr(my_bind_data->col_layer_ptr, current_layer);
        // Ugh, TODO: This won't be the same when we'll have sorted-by-agg RLE.
        my_bind_data->number_of_records = ((uint64)final_ptr - (uint64)my_bind_data->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);

        duckdb_bind_set_bind_data(info, my_bind_data, free);
    }

    void traceprov_duckdb_init(duckdb_init_info info){

        auto init_data_inst = (TraceProvInitData *)malloc(sizeof(TraceProvInitData));
        init_data_inst->current = 0;
        duckdb_init_set_init_data(info, init_data_inst, free);
    }

    uint64 fillup_pointer(
        struct traceprov_aggregate_layer *layer,
        duckdb_data_chunk chunk,
        void *source_ptr,
        uint64_t current_pos,
        const uint64 final_num_records,
        const uint64 start_width,
        const uint64 total_width,
        void **final_ptr
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
            current_pos++;
            source_ptr = (void *)canonical_ptr;
        }

        duckdb_data_chunk_set_size(chunk, current_pos - original_pos);
        *final_ptr = source_ptr;
        return current_pos;
    }


    void traceprov_duckdb_func(duckdb_function_info info, duckdb_data_chunk output){

        auto bind_data = (TraceProvBindData *)duckdb_function_get_bind_data(info);
        auto init_data = (TraceProvInitData *)duckdb_function_get_init_data(info);
        auto final_state = fillup_pointer(bind_data->col_layer_info, output, bind_data->col_layer_ptr, init_data->current, bind_data->number_of_records, 0, bind_data->column_width, &bind_data->col_layer_ptr);
        if (bind_data->row_layer_info){
            fillup_pointer(bind_data->row_layer_info, output, bind_data->row_layer_ptr, init_data->current, bind_data->number_of_records, bind_data->column_width, bind_data->row_width, &bind_data->row_layer_ptr);
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


    static duckdb_table_function setup_func(){
        auto function = duckdb_create_table_function();
        duckdb_table_function_set_name(function, "traceprov_read_worker_layer");
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
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
                traceprov_data->push_back(column_data);
            }
        }
        for (uint64 col_idx = 0; col_idx < column_count; col_idx++){
            duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
            uint64 *col_data = (uint64 *)duckdb_vector_get_data(col);
            uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
            auto current_column_data = traceprov_data->at(col_idx)->data;
            for (uint64 row_idx = 0; row_idx < row_count; row_idx++){
                if(unlikely(!duckdb_validity_row_is_valid(col_validity, row_idx))){
                    elog(ERROR, "Expected all non-null row, for now");
                }
                current_column_data->push_back(col_data[row_idx]);
            }
        }
    }

    // guts of all the inference.
    TraceProvData *traceprov_perform_duckdb_inference(char *generated_sql){
        duckdb_database db;
        duckdb_connection con;
        char *error_msg;
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(":memory:", &db, nullptr, &error_msg), error_msg);
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));
        auto function = setup_func();
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
        PG_DUCKDB_RUN_SHORT_QUERY(con, "SET threads=12;", "setting threads");

        duckdb_prepared_statement stmt;
        duckdb_result final_result;
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, generated_sql, &stmt), duckdb_prepare_error(stmt));
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));

        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        auto traceprov_data = new TraceProvData;
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++){
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            populate_traceprov_data(traceprov_data, &data_chunk);
            duckdb_destroy_data_chunk(&data_chunk);
        }

        PG_DUCKDB_RUN_SHORT_QUERY(con, generated_sql, "inference query");
        duckdb_disconnect(&con);
        duckdb_close(&db);
        return traceprov_data;
    }
}