#ifndef __TP_INFER__
#define __TP_INFER__
#include <vector>

#include <unordered_map>
#include "traceprov_infer_essentials.hpp"
#include "traceprov_node.hpp"

extern "C" {
    #include "traceprov_parse_context.h"
    #include "funcapi.h"

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
}

#endif