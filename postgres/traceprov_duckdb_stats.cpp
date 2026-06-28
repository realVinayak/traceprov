// Given that we use duckdb.h instead of the hpp for Postgres, need to "tap" into some of the hpp logic.
// This exposes the newer API that then the DuckDB table calls.
// Basically, a light-weight wrapper for the stats.

#include "table_bind_data.hpp"
#include "traceprov_duckdb_stats.hpp"
#include "traceprov_settings.h"
#include "traceprov_bind.hpp"
#include "tp_exprns.hpp"
extern "C" {

unique_ptr<BaseStatistics> traceprov_duckdb_table_stats(
    ClientContext &context,
    const FunctionData *bind_data,
    column_t column_index
);

static void traceprov_pushdown(ClientContext &context, LogicalGet &get,
                                                         FunctionData *bind_data,
                                                         vector<unique_ptr<Expression>> &filters){
    if (!traceprov_use_filter_pushdown || filters.size() > 1) return;
    // for (uint32_t idx = 0; idx < filters.size(); idx++){
    //     filters.at(idx).get()->Print();
    //     elog(INFO, "Filter content: %s, type: %ld",  filters.at(idx).get()->ToString().c_str(), filters.at(idx).get()->type);
    // }
    if (filters.size() == 1){
        auto &main_filter = filters[0];
        auto value = main_filter.get()->GetExpressionClass();
        if (value != duckdb::ExpressionClass::BOUND_COMPARISON){
            return;
        }
        auto comparison = reinterpret_cast<TraceProvBoundComparison *>(main_filter.get());
        if (comparison->type != ExpressionType::COMPARE_EQUAL) return;
        auto left_value = comparison->left.get();
        auto left_expression_class = left_value->GetExpressionClass();
        if (left_expression_class != duckdb::ExpressionClass::BOUND_COLUMN_REF) return;
        const auto column = reinterpret_cast<BoundColumnRefExpression *>(left_value)->binding.column_index;
        if (column != 0) return;
        auto right_value = comparison->right.get();
        auto right_expression_class = right_value->GetExpressionClass();
        const bool is_constant = right_expression_class == duckdb::ExpressionClass::BOUND_CONSTANT;
        if (!is_constant){
            return;
        }
        const auto right_const_value = reinterpret_cast<TraceProvBoundConstantExpression *>(right_value);
        const auto right_const_can = UBigIntValue::Get(right_const_value->value);
        TraceProvBindData *tp_bind_data = (TraceProvBindData *)((TraceProvCTableBindData *)bind_data)->bind_data;
        tp_bind_data->filter_value = right_const_can;
        tp_bind_data->filter_value_set = true;
        auto column_stats = traceprov_duckdb_table_stats(context, bind_data, 0);
        if (column_stats != nullptr){
            const auto stats = column_stats.get();
            uint64_t min_value = EMPTY_VALUE;
            uint64_t max_value = EMPTY_VALUE;
            if (stats->GetType() == LogicalType::UBIGINT || stats->GetType() == LogicalType::UINTEGER){
                if(NumericStats::HasMax(*stats)){
                    if (stats->GetType() == LogicalType::UBIGINT ){
                        max_value = NumericStats::GetMax<uint64_t>(*stats);
                    }else{
                        max_value = NumericStats::GetMax<uint32_t>(*stats);
                    }
                }
                if(NumericStats::HasMin(*stats)){
                    if (stats->GetType() == LogicalType::UBIGINT ){
                        min_value = NumericStats::GetMin<uint64_t>(*stats);
                    }else{
                        min_value = NumericStats::GetMin<uint32_t>(*stats);
                    }
                }
            }
            const bool is_valid = (min_value == EMPTY_VALUE || right_const_can >= min_value) && (max_value == EMPTY_VALUE || right_const_can <= max_value);
            tp_bind_data->is_dummy = !is_valid;
        }
        filters.clear();
    }
}


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
        auto duckdb_func = ((duckdb::TableFunction *)(table_func));
        duckdb_func->statistics = traceprov_duckdb_table_stats;
        duckdb_func->pushdown_complex_filter = traceprov_use_filter_pushdown == NULL ? NULL : traceprov_pushdown;
        duckdb_table_function_set_extra_info(table_func, (void *) get_stats, nullptr);
    }
}