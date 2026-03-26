#ifndef __TP_INFER__
#define __TP_INFER__
#include <vector>

#include <unordered_map>
#include "traceprov_infer_essentials.hpp"
#include "traceprov_node.hpp"

extern "C" {
    #include "traceprov_parse_context.h"
    #include "funcapi.h"
    #include "duckdb.h"

    struct traceprov_inference_context {
        duckdb_database db;
        duckdb_connection con;
    };

    TraceProvData *traceprov_perform_duckdb_inference(const char *generated_sql, struct traceprov_inference_context *context);
    void traceprov_duckdb_setup_context(
        struct traceprov_inference_context **p_ctxt,
        void (**p_cleanup)(struct traceprov_inference_context *),
        TraceProvInferSetupExtra *setup_extra
    );

    TraceProvInferResult traceprov_perform_duckdb_inference_pg_copy(
        const char *generated_sql,
        struct traceprov_inference_context *context,
        FunctionCallInfo fcinfo,
        const uint32 expected_col_width
    );

    typedef struct TraceProvRelationInferExtraItem {
        const char *sql;
        uint64_t expected_col_width;
    } TraceProvRelationInferExtraItem;

    typedef struct TraceProvRelationInferExtra {
        std::unordered_map<TraceProvLayerNumber, TraceProvRelationInferExtraItem> *map;
    } TraceProvRelationInferExtra;

    extern TraceProvRelationInferExtra g_tp_relation_infer_extra;

    // Format of the infer table.
    #define TRACEPROV_RELATION_INFER_NAME "traceprov_relation_infer_%d"

    #define PG_DUCKDB_EXIT_ON_ERROR(state) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d", __FILE__,  __LINE__); \
        } \
    }

    #define PG_DUCKDB_EXIT_ON_ERROR_MSG(state, msg) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__,  __LINE__, msg); \
        } \
    }

    #define PG_DUCKDB_EXIT_ON_ERROR_RESULT(state, result) { \
        if (state == DuckDBError){ \
            elog(ERROR, "Received duckdberror state at %s : %d (%s)", __FILE__,  __LINE__, duckdb_result_error(&result)); \
        } \
    }
}

#endif