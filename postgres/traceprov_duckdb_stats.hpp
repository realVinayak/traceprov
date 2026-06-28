#ifndef __TP_DUCKDB_STATS__
#define __TP_DUCKDB_STATS__
extern "C" {
typedef bool (*traceprov_stats_function)(void *bind_data, const int column_idx, uint64_t *min_value, uint64_t *max_value, uint64_t *distinct_count);
void traceprov_duckdb_table_set_stats_function(void *table_func, traceprov_stats_function get_stats);
#define EMPTY_VALUE ((uint64_t)(-1))
}
#endif