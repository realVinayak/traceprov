#include "rewriter_utils.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_clause.h"
#include "parser/analyze.h"

static Query *perform_window_clause_rewrite(Query *base, WindowClause *window_clause, List **extra_targets, TraceProvParseContext *context);
static List *remove_window_clause(List *original_clauses, const WindowClause *window_clause);

// main entrypoint for window rewriting.
// Postgres already sets up important things for us
// (like copying window defs, sort group clauses, setting up winrefs.)
// So, this is not _that_ much pain...
// The idea is to perform rewrite one window clause at a time.
// Since the number of rows are not changed, and references are still valid,
// this works pretty good. In this case, we do let Postgres decide whatever the
// optimum sort order. Although, we could do it by ourselves too....
// Note that any sort is removed, and reapplied at the top level.
// Since the sort exprns don't have any intrinsic ordering (that is, they are identified via tleSortGroupRef),
// moving them around is fine....
Query *traceprov_perform_window_rewrite(
    Query *base,
    List **extra_targets,
    TraceProvParseContext *context,
    List **traceprov_targets_per_rte
){
    if (!base->hasWindowFuncs)
        elog(ERROR, "Expected window functions to be present for rewriting traceprov!");

    const List *original_sort_clause = list_copy_deep(base->sortClause);
    Query *top_query = NULL;
    Query *last_query = NULL;
    ListCell *clause_cursor = NULL;
    foreach(clause_cursor, base->windowClause){
        WindowClause *wc = (WindowClause *)lfirst(clause_cursor);
        // We always modify the base query (it keeps on getting nested till there are no window clauses are present.)
        Query *created = perform_window_clause_rewrite(base, wc, NIL, context);
        if (top_query == NULL){
            // first iteration.
            top_query = created;
        }else if (last_query != NULL){
            ((RangeTblEntry *)lfirst(list_head(last_query->rtable)))->subquery = created;
        }
        last_query = created;
    }
}

static Query *perform_window_clause_rewrite(
    Query *base,
    WindowClause *input_window_clause,
    List **extra_targets,
    TraceProvParseContext *context
){
    // Regardless of frame option, we need to always nest the query in an order by.
    // The sort is not done explictly, since it can be reordered.
    // So, need to establish a "baseline", which is done by using the row_number() over (sort_clause),
    // and all future ops are done by sorting on it (for rows)
    List *sort_clause = list_concat_copy(
        list_copy_deep(input_window_clause->partitionClause),
        list_copy_deep(input_window_clause->orderClause)
    );

    WindowDef *window_def = makeNode(WindowDef);
    window_def->orderClause = sort_clause;
    Node *fc_node = traceprov_get_function_call_node(TRACEPROV_ROW_NUMBER, NIL, window_def);
    if (!IsA(fc_node, WindowFunc))
        elog(ERROR, "Expected the row_number call to be function call!");

    if (window_def->orderClause == NIL)
        elog(ERROR, "Got the order clause corrupted!");

    WindowClause *window_clause = makeNode(WindowClause);
    window_clause->orderClause = window_def->orderClause;
    // TODO: Try implementing checking if the window clause is used before.
    base->windowClause = lappend(base->windowClause, window_clause);
    window_clause->winref = list_length(base->windowClause);
    WindowFunc *window_fc_node = (WindowFunc *) fc_node;
    window_fc_node->winref = window_clause->winref;

    TargetEntry *ordered_row_number_te = makeTargetEntry((Expr *)window_fc_node, 0, pstrdup("ordered_row_number"), false);
    base->targetList = traceprov_append_at_resjunk(
        base->targetList,
        ordered_row_number_te
    );

    List *window_funcs_target_list = NIL;
    ListCell *target_entry_cursor = NULL;
    foreach(target_entry_cursor, base->targetList){
        TargetEntry *te = lfirst(target_entry_cursor);
        if (IsA(te->expr, WindowFunc)){
            WindowFunc *window_func = (WindowFunc *)(te->expr);
            if (window_func->winref == window_clause->winref){
                // If this is one of the window functions belonging to the current clause,
                // replace the current expr with a null.
                // But, remember this target entry.
                TargetEntry *old_te = copyObject(te);
                window_funcs_target_list = lappend(window_funcs_target_list, old_te);
                // So that we aren't referencing resjunk columns.
                // BUT, we need to actually refer to these colimns.
                te->resjunk = false;
                // Make this a null constant (of the same type as the previous te.)
                // This is done like this so that ordering doesn't get messed up.
                te->expr = makeNullConst(
                    exprType(old_te->expr),
                    exprTypmod(old_te->expr),
                    exprCollation(old_te->expr)
                );
            }
        }
    }

    // Need to remove the current window clause from the base too.
    List *original_window_clauses = list_copy_deep(base->windowClause);
    base->windowClause = remove_window_clause(base->windowClause, input_window_clause);

    Query *nested = traceprov_make_nested_query(base, context, true, true);

    TargetEntry *outer_ordered_row_number_te = list_nth(nested->targetList, ordered_row_number_te->resno - 1);
    if (outer_ordered_row_number_te->resno != ordered_row_number_te->resno){
        elog(ERROR, "Expected the ordered row number to be of the same resno!");
    }
    outer_ordered_row_number_te->ressortgroupref = assignSortGroupRef(outer_ordered_row_number_te, nested->targetList);
    SortGroupClause *ordered_row_number_sgc = makeSortGroupClauseForSetOp(exprType((Node*)outer_ordered_row_number_te->expr), false);
    ordered_row_number_sgc->tleSortGroupRef = outer_ordered_row_number_te->ressortgroupref;

    nested->windowClause = original_window_clauses;

    if (input_window_clause->frameOptions & FRAMEOPTION_ROWS){
        // If it is window, need to append the current input window clause's orderings with the ordered row number.
        input_window_clause->orderClause = lappend(input_window_clause->orderClause, ordered_row_number_sgc);
        WindowDef *rows_window_def = makeNode(WindowDef);

        Node *first_value_fc_node = traceprov_get_function_call_node(TRACEPROV_FIRST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
        if (!IsA(first_value_fc_node, WindowFunc))
            elog(ERROR, "Expected the row_number call to be function call!");

        Node *last_value_fc_node = traceprov_get_function_call_node(TRACEPROV_LAST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
        if (!IsA(last_value_fc_node, WindowFunc))
            elog(ERROR, "Expected the row_number call to be function call!");

        nested->targetList = traceprov_append_at_resjunk(nested->targetList, makeTargetEntry((Expr *)first_value_fc_node, 0, pstrdup("frame_start"), false));
        nested->targetList = traceprov_append_at_resjunk(nested->targetList, makeTargetEntry((Expr *)last_value_fc_node, 0, pstrdup("frame_end"), false));
    }

    return nested;
}

static List *remove_window_clause(List *original_clauses, const WindowClause *window_clause){
    List *new_clauses = NIL;
    ListCell *cursor = NULL;
    foreach(cursor, original_clauses){
        WindowClause *wc = (WindowClause *)lfirst(cursor);
        if (wc->winref != window_clause->winref)
            new_clauses = lappend(new_clauses, wc);
    }
    return new_clauses;
}