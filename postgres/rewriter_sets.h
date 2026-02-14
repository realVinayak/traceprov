#include "c.h"
#include "postgres.h"
#include "optimizer/planner.h"
#include "fmgr.h"
#include "traceprov_parse_context.h"
#include "rewriter_utils.h"
#include "nodes/makefuncs.h"

List *traceprov_adjust_union(
    SetOperationStmt *, 
    List *,
    List *,
    TraceProvParseContext *
);

Query *traceprov_adjust_except(
    Query *,
    List *,
    TraceProvParseContext *,
    bool,
    // The created extra targets (AFTER the reverse-type cast.)
    List **
);

List *traceprov_adjust_intersect(Query *, List *, TraceProvParseContext *);
