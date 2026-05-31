/** extra duckdb funcs for microbenchs */
// Putting in a separate file to not muddy the waters

#include "utils.hpp"
#include "traceprov_extra_funcs.hpp"
#include "traceprov.hpp"

extern "C" {
    #include <mem_alloc.h>
}

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
    duckdb_bind_set_cardinality(info, 1000*1000, true);
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
    sleep(3);
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

typedef struct TraceProvChunkedReadBind {
    int64_t layer_number;
    int64_t chunk_size;
    int64_t row_count;
    int64_t base_row_count;
    int64_t selectivity;
} TraceProvChunkedReadBind;

typedef struct TraceProvChunkedReadInit {
    int64_t rows_to_emit;
    int64_t extra_rows;
    int64_t index_in_build;
    int64_t total_rows_emitted;
} TraceProvChunkedReadInit;

void traceprov_chunked_read_bind(duckdb_bind_info info){
    auto param_layer_number = duckdb_bind_get_parameter(info, 0);
    const int64_t layer_number = duckdb_get_int64(param_layer_number);
    duckdb_destroy_value(&param_layer_number);

    auto param_chunk_size = duckdb_bind_get_parameter(info, 1);
    const int64_t chunk_size = duckdb_get_int64(param_chunk_size);
    duckdb_destroy_value(&param_chunk_size);

    auto param_row_count = duckdb_bind_get_parameter(info, 2);
    const int64_t row_count = duckdb_get_int64(param_row_count);
    duckdb_destroy_value(&param_row_count);

    auto param_base_row_count = duckdb_bind_get_parameter(info, 3);
    const int64_t base_row_count = duckdb_get_int64(param_base_row_count);
    duckdb_destroy_value(&param_base_row_count);

    auto param_selectivity = duckdb_bind_get_parameter(info, 4);
    const int64_t selectivity = duckdb_get_int64(param_selectivity);
    duckdb_destroy_value(&param_selectivity);

    auto bind_data = tp_alloc0_object(TraceProvChunkedReadBind);
    bind_data->layer_number = layer_number;
    bind_data->chunk_size = chunk_size;
    bind_data->row_count = row_count;
    bind_data->base_row_count = base_row_count;
    bind_data->selectivity = selectivity;

    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_bind_add_result_column(info, "column_0", type);
    duckdb_bind_add_result_column(info, "column_1", type);
    duckdb_destroy_logical_type(&type);

    duckdb_bind_set_cardinality(info, row_count, true);
    duckdb_bind_set_bind_data(info, bind_data, free);
}

void traceprov_chunked_read_init(duckdb_init_info info){
    auto init_data = tp_alloc0_object(TraceProvChunkedReadInit);
    auto bind_data = (TraceProvChunkedReadBind *)duckdb_init_get_bind_data(info);
    const uint64_t rows_to_emit = (bind_data->selectivity * bind_data->row_count) / 100;
    if (rows_to_emit == 0){
        elog(ERROR, "Expected to have some rows to emit!");
    }
    const uint64_t total_chunk_count = ((rows_to_emit - 1) / bind_data->chunk_size) + 1;
    const uint64_t extra_rows = (bind_data->row_count - rows_to_emit);
    const uint64_t extra_row_per_chunk = (extra_rows / total_chunk_count);
    init_data->rows_to_emit = rows_to_emit;
    init_data->extra_rows = extra_rows;
    init_data->index_in_build = 0;
    duckdb_init_set_init_data(info, init_data, free);
}

void traceprov_chunked_read(duckdb_function_info info, duckdb_data_chunk output){
    const auto bind_data = (TraceProvChunkedReadBind *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvChunkedReadInit *)duckdb_function_get_init_data(info);
    int64_t rows_to_emit = 0;
    int64_t extra_rows_to_emit = 0;
    // terminal case.
    if (init_data->total_rows_emitted > bind_data->row_count){
        return;
    }
    rows_to_emit = MIN(init_data->rows_to_emit, bind_data->chunk_size);
    extra_rows_to_emit = MIN(init_data->extra_rows, (STANDARD_VECTOR_SIZE - rows_to_emit));
    const int64_t total_rows_emitted = rows_to_emit + extra_rows_to_emit;
    int64_t *dest_ptr = (int64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, 1));
    int64_t *series_dest_ptr = (int64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, 0));
    int64_t row_value = init_data->index_in_build++;
    if (init_data->index_in_build > bind_data->base_row_count){
        elog(ERROR, "Got invalid state for index building!");
    }
    init_data->index_in_build = init_data->index_in_build % bind_data->base_row_count;
    for (int64_t idx = 0; idx < total_rows_emitted; idx++){
        if (idx >= rows_to_emit){
            row_value = TRACEPROV_SET_WORKER_ID((init_data->total_rows_emitted + idx), bind_data->layer_number);
        }
        dest_ptr[idx] = row_value;
        series_dest_ptr[idx] = init_data->total_rows_emitted + idx;
    }
    init_data->total_rows_emitted += total_rows_emitted;
    init_data->rows_to_emit -= rows_to_emit;
    init_data->extra_rows -= extra_rows_to_emit;
    if (init_data->rows_to_emit < 0 || init_data->extra_rows < 0){
        elog(ERROR, "Underflow is impossible!");
    }
    duckdb_data_chunk_set_size(output, total_rows_emitted);
}


void traceprov_create_chunk_table_func(duckdb_connection con){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "traceprov_chunked_read");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, type); // layer number
    duckdb_table_function_add_parameter(function, type); // chunk size
    duckdb_table_function_add_parameter(function, type); // row count
    duckdb_table_function_add_parameter(function, type); // base row count
    duckdb_table_function_add_parameter(function, type); // selectivity
    duckdb_destroy_logical_type(&type);

    duckdb_table_function_set_bind(function, traceprov_chunked_read_bind);
    duckdb_table_function_set_init(function, traceprov_chunked_read_init);
    duckdb_table_function_set_function(function, traceprov_chunked_read);

    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
}
