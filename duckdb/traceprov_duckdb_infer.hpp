// TODO: Make this .h?
extern "C" {
    #include "duckdb.h"
    duckdb_table_function traceprov_create_table_func();
    duckdb_table_function traceprov_create_table_offset_func();
    #if TRACEPROV_SD_MODE == 0
    duckdb_scalar_function traceprov_create_table_window_func(const uint32_t num_args, const uint32_t worker_count, std::vector<uint32_t> *expected_layers);
    #endif
}