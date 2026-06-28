#ifndef __TP_INFER__
#define __TP_INFER__
#include <vector>

#include <unordered_map>
#include "traceprov_infer_essentials.hpp"
#include "traceprov_node.hpp"

extern "C"
{
#include "traceprov_parse_context.h"
#include "funcapi.h"
#include "duckdb.h"

    struct traceprov_inference_context
    {
        duckdb_database db;
        duckdb_connection con;
    };

    TraceProvData *traceprov_perform_duckdb_inference(const char *generated_sql, struct traceprov_inference_context *context);
    void traceprov_duckdb_setup_context(
        struct traceprov_inference_context **p_ctxt,
        void (**p_cleanup)(struct traceprov_inference_context *),
        TraceProvInferSetupExtra *setup_extra);

    TraceProvInferResult traceprov_perform_duckdb_inference_pg_copy(
        const char *generated_sql,
        struct traceprov_inference_context *context,
        FunctionCallInfo fcinfo,
        const uint32 expected_col_width);

    typedef enum TraceProvInferType {
        FOLDABLE=0,
        SQL
    } TraceProvInferType;

    // TODO: Use union types?
    typedef struct TraceProvRelationInferExtraItem
    {
        TraceProvInferType tag;
        uint64_t expected_col_width;
        // this gets used when it is a SQL
        const char *sql;
        // These get used when it is foldable
        TraceProvRelationArgs *rel_args;
        uint64_t filter_value;
        bool use_filter_value;
        uint64_t *foldable_values;
        uint64_t foldable_value_count;
    } TraceProvRelationInferExtraItem;

    typedef std::unordered_map<TraceProvLayerNumber, int64_t> TraceProvLogOffsetMap;

    typedef struct TraceProvRelationInferExtra
    {
        std::unordered_map<TraceProvLayerNumber, TraceProvRelationInferExtraItem> *map;
        TraceProvLogOffsetMap *log_offset;
    } TraceProvRelationInferExtra;

    void *traceprov_get_row(
        const uint64_t layer_number,
        const uint64_t log_probe_value
    );

    void initialize_g_tp_duckdb_state();

    std::vector<TraceProvWorkerLayer> *find_layers_across_workers(
        TraceProvLayerNumber log_layer_number,
        const std::vector<struct local_context *> *worker_local_contexts,
        const uint32 expected_layer_width,
        const bool do_strict = true);

    extern TraceProvRelationInferExtra g_tp_relation_infer_extra;

    void traceprov_duckdb_func_core(
        TraceProvBindData *bind_data_combined,
        TraceProvInitData *init_data_combined,
        duckdb_data_chunk output
    );

    void traceprov_prepare_foldable(
        TraceProvRelationInferExtraItem *item,
        TraceProvBindData **bind_data_core,
        TraceProvInitData **init_data_core,
        TraceProvInitData **local_init_data
    );
    // void traceprov_prepare_foldable(
    //     const TraceProvRelationArgs *rel_args,
    //     uint64_t filter_value,
    //     bool use_filter_value,
    //     TraceProvBindData **bind_data_core,
    //     TraceProvInitData **init_data_core,
    //     TraceProvInitData **local_init_data
    // );

// Format of the infer table.
#define TRACEPROV_RELATION_INFER_NAME "traceprov_relation_infer_%d"

#define PG_DUCKDB_EXIT_ON_ERROR(state)                                                \
    {                                                                                 \
        if (state == DuckDBError)                                                     \
        {                                                                             \
            elog(ERROR, "Received duckdberror state at %s : %d", __FILE__, __LINE__); \
        }                                                                             \
    }

#define PG_DUCKDB_EXIT_ON_ERROR_MSG(state, msg)                                                 \
    {                                                                                           \
        if (state == DuckDBError)                                                               \
        {                                                                                       \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__, __LINE__, msg); \
        }                                                                                       \
    }

#define PG_DUCKDB_EXIT_ON_ERROR_RESULT(state, result)                                                                    \
    {                                                                                                                    \
        if (state == DuckDBError)                                                                                        \
        {                                                                                                                \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__, __LINE__, duckdb_result_error(&result)); \
        }                                                                                                                \
    }
}

#endif