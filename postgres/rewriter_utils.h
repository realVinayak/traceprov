#ifndef __TRACEPROV_REWRITER_UTILS__
#define __TRACEPROV_REWRITER_UTILS__
#include "postgres.h"
#include "c.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "traceprov_parse_context.h"

// The function name to use, for aggregation over simple primary keys.
#define TRACEPROV_AGG_FUNC_NAME "traceprov_agg_key_parallel"
#define TRACEPROV_MARK_LATER_FUNC_NAME "mark_later"
#define TRACEPROV_MARK_LATER_VALUE_FUNC_NAME "mark_later_value"
// in cases where aggregate is nested, we need to get the offsets directly.
#define TRACEPROV_AGG_OFFSETS_FUNC_NAME "traceprov_agg_key_parallel_offset"
#define TRACEPROV_POINTER_TYPE_NAME "traceprov_ptr_type"

// Space to log entries.
#define TRACEPROV_LOG_FUNC_NAME "traceprov_log_entry"

// volatile version of traceprov_log_entry.
// This is used if we're in a sublink (where we'd want the side-effect of logging to be seen)
// Basically, sublinks create a volatility "barrier" for logs (everything inside of this is always stable, until we hit another sublink)
#define TRACEPROV_LOG_VOLATILE_FUNC_NAME "traceprov_log_entry_volatile"

typedef struct TraceProvUsedRefNavigator {
    bool includeInternals;
    bool goLeft;
    bool goRight;
} TraceProvUsedRefNavigator;

static TraceProvUsedRefNavigator traceProvInclusiveNavigator = {
    .includeInternals = true,
    .goLeft = true,
    .goRight = true
};

static TraceProvUsedRefNavigator traceProvLeafNavigator = {
    .includeInternals = false,
    .goLeft = true,
    .goRight = true
};

void traceProvAssertNoResJunk(const List *);
void traceProvAssertIsSubquery(const RangeTblEntry *);
int traceProvAssertEqualLength(List *);

List *traceProvAppendAtResJunk(List *, TargetEntry *);
List *traceProvGetNullList(unsigned, Oid, int32, Oid);
List *traceProvFindUsedRefs(Node *, TraceProvUsedRefNavigator);

List *traceProvDupInt(int, int);
List *traceProvDupOid(Oid, int);

List *traceprov_flatten(List *);
List* traceprov_append_targets(List *, List *);

const TraceProvTarget *traceProvFindMatchingSetPointer(List *, int);

Node *createEqualityCondition (List*, List*, Index, Index, bool);

Query *traceprov_clone_query(const Query *);
RangeTblEntry *rangeTableEntryFromSubquery(Query *, TraceProvParseContext *);
Query *traceprov_make_nested_query(Query *, TraceProvParseContext *);

List *traceprov_aggregate_on_set(
    const SetOperationStmt *, 
    Query *,
    TraceProvParseContext *,
    bool,
    List *,
    List *
);

bool traceProvFindIntList(List *, int);

void traceprov_aggregate_rewrite(
    const List *,
    List **,
    TraceProvParseContext *,
    bool
);

Node *getFunctionCallNode(const char *, List *);

Const *makeInt8Const(int64);

List *traceProvPropagateChildTargets(List *, Index);
char *traceprovParseBackQuery(Query *);

Query *traceprov_perform_rewrite(
    Query *, 
    List **,
    TraceProvParseContext *,
    bool
);

#endif