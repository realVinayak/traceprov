// TODO: Make this .h?
extern "C" {
    #include "duckdb.h"
    duckdb_table_function traceprov_create_table_func();
    duckdb_table_function traceprov_create_table_offset_func();
}