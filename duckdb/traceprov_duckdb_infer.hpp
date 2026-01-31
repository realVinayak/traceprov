// TODO: Make this .h?
extern "C" {
    #include "duckdb.h"
    duckdb_table_function traceprov_create_table_func();
    duckdb_table_function traceprov_create_table_offset_func();
    duckdb_scalar_function traceprov_create_table_window_func(const uint32_t num_args);
}