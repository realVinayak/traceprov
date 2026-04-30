// Given that we use duckdb.h instead of the hpp for Postgres, need to "tap" into some of the hpp logic.
// This exposes the newer API that then the DuckDB table calls.
// Basically, a light-weight wrapper for the stats.

#include "table_bind_data.hpp"
#include "traceprov_duckdb_stats.hpp"
extern "C" {

    unique_ptr<BaseStatistics> traceprov_duckdb_table_stats(
        ClientContext &context,
        const FunctionData *bind_data,
        column_t column_index
    ){
        auto ctable_bind_data = ((TraceProvCTableBindData *)bind_data);
        traceprov_stats_function stats = (traceprov_stats_function)(ctable_bind_data->info.extra_info);
        uint64_t min_value = 0, max_value = 0, distinct_count = 0;
        const bool did_set = stats(ctable_bind_data->bind_data, (int) column_index, &min_value, &max_value, &distinct_count);
        auto result = NumericStats::CreateEmpty(LogicalType::UBIGINT);
        if (did_set){
            NumericStats::SetMin(result, min_value);
            NumericStats::SetMax(result, max_value);
            if (distinct_count){
                result.SetDistinctCount(distinct_count);
            }
        }
        return result.ToUnique();
    }

    void traceprov_duckdb_table_set_stats_function(void *p_table_func, traceprov_stats_function get_stats){
        auto table_func = (duckdb_table_function )p_table_func;
        ((duckdb::TableFunction *)(table_func))->statistics = traceprov_duckdb_table_stats;
        duckdb_table_function_set_extra_info(table_func, (void *) get_stats, nullptr);
    }
}