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

// These are all the same as postgres' implementation.
#define TRACEPROV_ROW_NUMBER "row_number"
#define TRACEPROV_FIRST_VALUE "first_value"
#define TRACEPROV_LAST_VALUE "last_value"

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

extern TraceProvUsedRefNavigator traceProvInclusiveNavigator;
extern TraceProvUsedRefNavigator traceProvLeafNavigator;

void traceprov_assert_no_resjunk(const List *);
void traceprov_assert_is_subquery(const RangeTblEntry *);
int traceprov_assert_equal_length(List *);

List *traceprov_append_at_resjunk(List *, TargetEntry *);
List *traceprov_get_null_list(unsigned, Oid, int32, Oid);
List *traceprov_find_used_refs(Node *, TraceProvUsedRefNavigator);

List *traceprov_dup_int(int, int);
List *traceprov_dup_oid(Oid, int);

List *traceprov_flatten(List *);
List* traceprov_append_targets(List *, List *);

const TraceProvTarget *traceprov_find_matching_set_pointer(List *, int);

Node *createEqualityCondition (List*, List*, Index, Index, bool);

Query *traceprov_clone_query(const Query *);
RangeTblEntry *range_table_entry_from_subquery(Query *, TraceProvParseContext *, bool copy_resjunk);
Query *traceprov_make_nested_query(Query *, TraceProvParseContext *, bool copy_resjunk, bool copy_sort_ref);

List *traceprov_aggregate_on_set(
    const SetOperationStmt *, 
    Query *,
    TraceProvParseContext *,
    bool,
    List *,
    List *
);

bool traceprov_find_int_list(List *, int);
bool traceprov_find_oid_list(List *, Oid);

void traceprov_aggregate_rewrite(
    const List *,
    List **,
    TraceProvParseContext *,
    bool,
    WindowDef *
);

Node *traceprov_get_function_call_node(const char *, List *, WindowDef *);

Const *makeInt8Const(int64);

List *traceprov_propagate_child_targets(List *, Index);
char *tracprov_parse_back_query(Query *);

Query *traceprov_perform_rewrite(
    Query *, 
    List **,
    TraceProvParseContext *,
    bool
);

List *pull_vars_of_level_ignore_sublinks(Node *, int );
List *traceprov_prepare_arg_vars(TraceProvParseContext *context, TraceProvLayerNumber *out_layer_number);

Query *traceprov_perform_window_rewrite(
    Query *base,
    List **extra_targets,
    TraceProvParseContext *context,
    List **traceprov_targets_per_rte
);

#endif