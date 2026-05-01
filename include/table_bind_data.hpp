#ifndef __TP_TABLE_BIND_DATA__
#define __TP_TABLE_BIND_DATA__
#include "duckdb.hpp"

using namespace duckdb;

struct TraceProvCTableFunctionInfo : public TableFunctionInfo {
    ~TraceProvCTableFunctionInfo() {
        if (extra_info && delete_callback) {
            delete_callback(extra_info);
        }
        extra_info = nullptr;
        delete_callback = nullptr;
    }

    duckdb_table_function_bind_t bind = nullptr;
    duckdb_table_function_init_t init = nullptr;
    duckdb_table_function_init_t local_init = nullptr;
    duckdb_table_function_t function = nullptr;
    void *extra_info = nullptr;
    duckdb_delete_callback_t delete_callback = nullptr;
};

struct TraceProvCTableBindData : public TableFunctionData {
    TraceProvCTableBindData(TraceProvCTableFunctionInfo &info) : info(info) {
    }
    ~TraceProvCTableBindData() {
        if (bind_data && delete_callback) {
            delete_callback(bind_data);
        }
        bind_data = nullptr;
        delete_callback = nullptr;
    }

    TraceProvCTableFunctionInfo &info;
    void *bind_data = nullptr;
    duckdb_delete_callback_t delete_callback = nullptr;
    unique_ptr<NodeStatistics> stats;
};

#endif