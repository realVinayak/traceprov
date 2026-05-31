#ifndef __TP_EXTRA_FUNCS__
#define __TP_EXTRA_FUNCS__
#include "duckdb.hpp"

void traceprov_create_vary_chunk_funcs(duckdb_connection con);
void traceprov_create_debug_table_funcs(duckdb_connection con);
void traceprov_create_chunk_table_func(duckdb_connection con);
#endif