#include "c.h"
#include "postgres.h"
#include "optimizer/planner.h"
#include "fmgr.h"
#include "traceprov_parse_context.h"
#include "rewriter_utils.h"
#include "nodes/makefuncs.h"

List *adjustUnionSetOps(
    SetOperationStmt *, 
    List *,
    List *,
    TraceProvParseContext *
);

Query *adjustExceptSetOps(
    Query *,
    List *,
    TraceProvParseContext *,
    bool,
    // The created extra targets (AFTER the reverse-type cast.)
    List **
);

Query *handleIntersect(Query *, List *);