#include <mutex>
extern "C" {
    #include "nodes/pg_list.h"
    #include "traceprov_node.hpp"
    typedef struct TraceProvBindData
    {
        TraceProvRelationArgs rel_args;
        struct traceprov_aggregate_layer *col_layer_info;
        struct traceprov_aggregate_layer *row_layer_info;
        struct traceprov_aggregate_layer *col_null_map_layer_info;
        struct traceprov_aggregate_layer *row_null_map_layer_info;
        uint64_t column_width;
        uint64_t row_width;
        std::vector<TraceProvBindData *> *child_bind_data;
        std::mutex *bind_data_mutex;
        uint64_t max_worker_idx;
        bool is_aggregate;
        bool is_combine;
        int64_t offset;
        uint64_t number_of_records;
        uint64_t number_of_col_records;
        uint64_t filter_value;
        uint64_t is_dummy;
        bool filter_value_set;
    } TraceProvBindData;
}