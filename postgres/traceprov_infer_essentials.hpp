#ifndef __TP_INFER_ESSENTIALS__
#define __TP_INFER_ESSENTIALS__

#include <stdint.h>

// These are completely indepdent of postgres or duckdb (based off stdint)
// THis is done this way so we don't have to directly map PG -> DUCKDB
// and vice versa.
typedef struct TraceProvRelationArgs {
    uint64_t worker_id;
    uint64_t layer_number;
} TraceProvRelationArgs;


#endif