// g++ -O3 -L /home/realvinayak123/projects/traceprov/postgres/ -g perform_duckdb_inference.cpp -l duckdb -o perform_duckdb_inference

#include <iostream>
#include <fstream>
#include <sstream>
#include "duckdb.hpp"
#include <vector>
#include <chrono>
#include <cstring>

#define MAX_NUM_COLUMNS 100

int main(int argc, char *argv[]){

    std::string sql_str;
    getline(std::cin, sql_str);
    const char *duckdb_str = sql_str.c_str();

    // const size_t size_init = strlen(duckdb_str);
    // for (int i = 0; i < size_init; i++){
    //     std::cout << "APPENDING: " << (duckdb_str[i]) << " at: " << i << std::endl;
    //     new_str += (duckdb_str[i]);
    //     new_str += ('\n');
    // }
    // new_str += '\0';
    std::cout << "Executing: " << duckdb_str << std::endl;

    duckdb_database db;
    duckdb_connection con;
    if (duckdb_open(NULL, &db) == DuckDBError){
        std::cout << "Error opening db" << std::endl;
        exit(1);
    }
    if (duckdb_connect(db, &con) == DuckDBError){
        std::cout << "Error doing a connection" << std::endl;
        exit(1);
    }

    duckdb_state state;
    duckdb_result result;

    state = duckdb_query(con, "INSTALL parquet;", &result);
    if (state == DuckDBError){
        std::cout << "Error running INSTALL" << std::endl;
        std::cout << duckdb_result_error(&result);
        exit(1);
    }
    duckdb_destroy_result(&result);

    std::vector<uint64_t> **derived_records = (std::vector<uint64_t> **)malloc(sizeof(std::vector<uint64_t> *)*MAX_NUM_COLUMNS);
    for (int i = 0; i < MAX_NUM_COLUMNS; i++){
        derived_records[i] = new std::vector<uint64_t>;
    }
    auto start = std::chrono::high_resolution_clock::now();
    state = duckdb_query(con, duckdb_str, &result);
    auto test_end = std::chrono::high_resolution_clock::now();
    auto duration_query = std::chrono::duration_cast<std::chrono::milliseconds>(test_end - start);
    std::cout << "Took: " << duration_query.count() << "(ms)" << std::endl;
    if (state == DuckDBError){
        std::cout << "Error running query" << std::endl;
        std::cout << duckdb_result_error(&result);
        exit(1);
    }

    int derived_column_count = -1;

    while (true) {
        duckdb_data_chunk data_chunk = duckdb_fetch_chunk(result);
        if (!data_chunk) break;
        const idx_t row_count = duckdb_data_chunk_get_size(data_chunk);
        const idx_t col_count = duckdb_data_chunk_get_column_count(data_chunk);
        derived_column_count = col_count;
        for (idx_t column = 0; column < col_count; column++){
            duckdb_vector col = duckdb_data_chunk_get_vector(data_chunk, column);
            uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
            uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
            
            std::vector<uint64_t> *column_derived_records = (derived_records[column]);
            for (idx_t row = 0; row < row_count; row++){
                if(!duckdb_validity_row_is_valid(col_validity, row)){
                    std::cout << "Expected all non-null row, for now" << std::endl;
                }
                column_derived_records->push_back(col_data[row]);
            }
        }
        duckdb_destroy_data_chunk(&data_chunk);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Took: " << duration.count() << "(ms)" << std::endl;
    if (derived_records[0]->size() == 0){
        std::cout << "Expected some records to be in provenance;" << std::endl;
        exit(1);
    }
    int row_count_derived = -1;
    if (derived_column_count < 1){
        std::cout << "Expected some columns (greater than 1)" << std::endl;
        exit(1);
    }
    for (int i = 0; i < derived_column_count; i++){
        const int current_size = derived_records[i]->size();
        if (row_count_derived != -1 && row_count_derived != current_size){
            std::cout << "Expected all the columns to be of the same num records!" << std::endl;
            exit(1);
        }
        row_count_derived = current_size;
    }
    if (row_count_derived < 1){
        std::cout << "Expected the derived row size to be >= 1" << std::endl;
        exit(1);
    }
    std::cout << "Final size: " << row_count_derived << std::endl;
    std::ofstream duckdb_cpp_inference("/tmp/duckdb_cpp_inference.txt");
    duckdb_cpp_inference << std::to_string(duration.count());
    duckdb_cpp_inference.close();

    for (int i = 0; i < 10 && i < derived_records[0]->size(); i++){
        std::cout << "OUT: " << derived_records[0]->at(i) << std::endl;
    }

    std::ofstream duckdb_cpp_inference_size("/tmp/duckdb_cpp_inference_size.txt");
    duckdb_cpp_inference_size << std::to_string(row_count_derived);
    duckdb_cpp_inference_size.close();

    std::ofstream duckdb_cpp_column_count("/tmp/duckdb_cpp_column_count.txt");
    duckdb_cpp_column_count << std::to_string(derived_column_count);
    duckdb_cpp_column_count.close();

    duckdb_destroy_result(&result);
    duckdb_disconnect(&con);
    duckdb_close(&db);
    return 0;
}