/** extra duckdb funcs for microbenchs */
// Putting in a separate file to not muddy the waters

#include "utils.hpp"
#include "traceprov_extra_funcs.hpp"
#include "traceprov.hpp"

#define Min(X, Y) (X > Y ? Y : X)

typedef struct TraceProvChunkInit {
    uint64_t offset;
} TraceProvChunkInit;

typedef struct TraceProvChunkBind {
    // Emit these many rows in total
    uint64_t num_rows;
    // Don't emit more than these many rows in a single chunk.
    uint64_t max_chunk_size;
    // How many columns to emit?
    uint64_t column_count;
} TraceProvChunkBind;

void traceprov_chunk_bind(duckdb_bind_info info){
    if (duckdb_bind_get_parameter_count(info) != 3){
        elog(ERROR, "Expected 3 params!");
    }
    auto param_1 = duckdb_bind_get_parameter(info, 0);
    const uint64_t num_rows = duckdb_get_int64(param_1);
    duckdb_destroy_value(&param_1);

    auto param_2 = duckdb_bind_get_parameter(info, 1);
    const uint64_t max_chunk_size = duckdb_get_int64(param_2);
    if (max_chunk_size > TP_STD_VECTOR_SIZE){
        elog(ERROR, "Cannot emit more than std vector size!");
    }
    duckdb_destroy_value(&param_2);

    auto param_3 = duckdb_bind_get_parameter(info, 2);
    const uint64_t column_count = duckdb_get_int64(param_3);
    duckdb_destroy_value(&param_3);

    auto bind_data = (TraceProvChunkBind *)malloc(sizeof(TraceProvChunkBind));
    bind_data->num_rows = num_rows;
    bind_data->max_chunk_size = max_chunk_size;
    bind_data->column_count = column_count;

    for (uint64_t col_count = 0; col_count < column_count; col_count++){
        const std::string param = std::string("column_") + std::to_string(col_count);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
        duckdb_bind_add_result_column(info, param.c_str(), type);
        duckdb_destroy_logical_type(&type);
    }

    duckdb_bind_set_bind_data(info, bind_data, free);
}


void traceprov_chunk_init(duckdb_init_info info){
    auto bind_data = (TraceProvChunkBind *)duckdb_init_get_bind_data(info);
    auto init_data = (TraceProvChunkInit *)malloc(sizeof(TraceProvChunkInit));
    init_data->offset = 0;
    duckdb_init_set_init_data(info, init_data, free);
}


void traceprov_chunk_func(duckdb_function_info info, duckdb_data_chunk output){
    auto bind_data = (TraceProvChunkBind *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvChunkInit *)duckdb_function_get_init_data(info);

    uint64_t chunk_size = 0;

    if (init_data->offset < bind_data->num_rows){
        chunk_size = Min((bind_data->num_rows - init_data->offset), bind_data->max_chunk_size);
        for (idx_t col_idx = 0; col_idx < bind_data->column_count; col_idx++){
            uint64_t *dest_ptr = (uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
            for (idx_t row_idx = 0; row_idx < chunk_size; row_idx++){
                dest_ptr[row_idx] += col_idx + 100;
            }
        }
        init_data->offset += chunk_size;
    }

    duckdb_data_chunk_set_size(output, chunk_size);
}

void traceprov_create_vary_chunk_funcs(duckdb_connection con){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "traceprov_vary_chunk");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_destroy_logical_type(&type);

    duckdb_table_function_set_bind(function, traceprov_chunk_bind);
    duckdb_table_function_set_init(function, traceprov_chunk_init);
    duckdb_table_function_set_function(function, traceprov_chunk_func);

    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
}


void traceprov_debug_table_bind(duckdb_bind_info info){
    auto param_1 = duckdb_bind_get_parameter(info, 0);
    const uint64_t num_rows = duckdb_get_int64(param_1);
    duckdb_destroy_value(&param_1);
    elog(INFO, "Calling bind for %ld, %d\n", num_rows, MyProcPid);
    duckdb_bind_set_bind_data(info, (void *)num_rows, nullptr);
    for (uint64_t col_count = 0; col_count < 3; col_count++){
        const std::string param = std::string("column_") + std::to_string(col_count);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
        duckdb_bind_add_result_column(info, param.c_str(), type);
        duckdb_destroy_logical_type(&type);
    }
    // duckdb_bind_set_cardinality(info, 1000*1000, true);
}


void traceprov_debug_table_init(duckdb_init_info info){
    duckdb_init_set_max_threads(info, 99999);
    auto bind_data = (uint64_t)duckdb_init_get_bind_data(info);
    elog(INFO, "Calling global init for %ld, %d:%d\n", bind_data, MyProcPid, MyProcTid);
    duckdb_init_set_init_data(info, (void *)bind_data, nullptr);
}

void traceprov_debug_table_local_init(duckdb_init_info info){
    auto bind_data = (uint64_t)duckdb_init_get_bind_data(info);
    elog(INFO, "Calling local init for %ld, %d:%d\n", bind_data, MyProcPid, MyProcTid);
    bind_data *= 10;
    duckdb_init_set_init_data(info, (void *)bind_data, nullptr);
}

void traceprov_debug_table_func(duckdb_function_info info, duckdb_data_chunk output){
    auto init_data =  (uint64_t)duckdb_function_get_init_data(info);
    auto local_init_data = (uint64_t)duckdb_function_get_local_init_data(info);
    elog(INFO, "Local init data: %ld, %ld, %d:%d\n", init_data, local_init_data, MyProcPid, MyProcTid);
    
}

// Just some functions for debugging
void traceprov_create_debug_table_funcs(duckdb_connection con){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "traceprov_debug_table");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
    duckdb_table_function_add_parameter(function, type);
    duckdb_destroy_logical_type(&type);

    duckdb_table_function_set_bind(function, traceprov_debug_table_bind);
    duckdb_table_function_set_init(function, traceprov_debug_table_init);
    duckdb_table_function_set_local_init(function, traceprov_debug_table_local_init);
    duckdb_table_function_set_function(function, traceprov_debug_table_func);

    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
}