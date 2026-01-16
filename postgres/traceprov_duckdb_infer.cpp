#ifndef TP_DUCKDB_INCLUDED
#include "duckdb.hpp"
#endif
#include "traceprov_infer_essentials.hpp"
#include "stdlib.h"
#include <iostream>
#include <string>


// Duckdb integration for inference.
// Duckdb doesn't technically do anything smart (just runs the query)
// The "leaf" are table scan functions for this mess.

#ifndef STANDARD_VECTOR_SIZE
#define STANDARD_VECTOR_SIZE 2048
#endif

struct TraceProvDuckInitData {
    int64_t offset;
};

void my_bind(duckdb_bind_info info) {
    if (duckdb_bind_get_parameter_count(info) != 2){
        std::cout << "Expected 2 params!";
        exit(1);
    }

    auto my_bind_data = (TraceProvRelationArgs *)malloc(sizeof(TraceProvRelationArgs));
    auto param_1 = duckdb_bind_get_parameter(info, 0);
    my_bind_data->worker_id = duckdb_get_int64(param_1);
    auto param_2 = duckdb_bind_get_parameter(info, 1);
    my_bind_data->layer_number = duckdb_get_int64(param_2);
    duckdb_destroy_value(&param_1);
    duckdb_destroy_value(&param_2);

    for (uint64_t col_count = 0; col_count < my_bind_data->layer_number; col_count++){
        const std::string param = std::string("param_") + std::to_string(col_count);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
        duckdb_bind_add_result_column(info, param.c_str(), type);
        duckdb_destroy_logical_type(&type);
    }

	duckdb_bind_set_bind_data(info, my_bind_data, free);
}

void my_init(duckdb_init_info info){
    auto init_data_inst = (TraceProvDuckInitData *)malloc(sizeof(TraceProvDuckInitData));
    init_data_inst->offset = 0;
    duckdb_init_set_init_data(info, init_data_inst, free);
}

uint64_t called_times = 0;

void my_func(duckdb_function_info info, duckdb_data_chunk output){
    called_times++;
    auto bind_data = (TraceProvRelationArgs *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvDuckInitData *)duckdb_function_get_init_data(info);

    const uint64_t original_current_offset = init_data->offset;
    for (uint64_t col_count = 0; col_count < bind_data->layer_number; col_count++){
        uint64_t current_pos = original_current_offset;
        auto ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_count));
        for (uint64_t i = 0; i < STANDARD_VECTOR_SIZE; i++){
            if (current_pos >= bind_data->worker_id){
                break;
            }
            ptr[i] = i;
            current_pos++;
        }
        // Only set it for the first column.
        if (col_count == 0)
            init_data->offset = current_pos;
    }
    duckdb_data_chunk_set_size(output, init_data->offset - original_current_offset);
    std::cout << "called: " << called_times << std::endl;
}

duckdb_table_function setup_func(){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "test_func");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_destroy_logical_type(&type);

	duckdb_table_function_set_bind(function, my_bind);
	duckdb_table_function_set_init(function, my_init);
	duckdb_table_function_set_function(function, my_func);

    return function;
}

// // Just for testing.
// int main(){
//     auto function = duckdb_create_table_function();
//     duckdb_table_function_set_name(function, "test_func");
//     duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
//     duckdb_table_function_add_parameter(function, type);
//     duckdb_destroy_logical_type(&type);

// 	duckdb_table_function_set_bind(function, my_bind);
// 	duckdb_table_function_set_init(function, my_init);
// 	duckdb_table_function_set_function(function, my_func);

//     status = duckdb_register_table_function(connection, function);
// }