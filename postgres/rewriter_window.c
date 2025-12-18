#include "rewriter_utils.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_clause.h"
#include "parser/analyze.h"
#include "optimizer/optimizer.h"

static Query *perform_window_clause_rewrite_inline(
    Query *query,
    List *window_clauses,
    TraceProvParseContext *context
);

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
    Query *top_query = perform_window_clause_rewrite_inline(base, base->windowClause, context);
    return top_query;
}

static void set_winref(Node *func, Index winref){
    if (!IsA(func, WindowFunc)) elog(ERROR, "Expected to be always called for window functions");

    WindowFunc *win_func = (WindowFunc *)func;
    win_func->winref = winref;
}

// Rewrites window functions.
static Query *perform_window_clause_rewrite_inline(
    Query *query,
    List *window_clauses,
    TraceProvParseContext *context
){
    // Push down the current query.
    // This is needed because, oth
    List *shift_spec = NIL;
    Query *subquery = traceprov_push_down_query(query, &shift_spec);

    // Adjust the current level vars to point to RTE 1.
    // This is done before we append new targets.
    List *current_vars = pull_vars_of_level((Node*)query, 0);
    ListCell *var_cursor = NULL;
    foreach(var_cursor, current_vars){
        Var *var = lfirst_node(Var, var_cursor);
        var->varattno += list_nth_int(shift_spec, var->varno - 1);
        var->varno = 1;
    }

    // Need to push down all the targets that are referenced in the window clauses.
    ListCell *window_clause_cursor = NULL;
    int last_win_ref = -1;
    Bitmapset *added_refs = NULL;

    foreach(window_clause_cursor, query->windowClause){
        WindowClause *wc = (WindowClause *)lfirst(window_clause_cursor);
        if ((int)wc->winref <= last_win_ref){
            elog(ERROR, "Trying to handle a previous winref!");
        }
        last_win_ref = wc->winref;

        List *sort_clause = list_concat_copy(
            list_copy_deep(wc->partitionClause),
            list_copy_deep(wc->orderClause)
        );

        ListCell *sort_clause_cursor = NULL;
        // Iteratre over the sort clause, and figure out the targets that
        // need to be pushed down.
        foreach(sort_clause_cursor, sort_clause){
            TargetEntry *te = get_sortgroupclause_tle(
                lfirst_node(SortGroupClause, sort_clause_cursor),
                query->targetList
            );
            // It is possible that the same target expr appears multiple times.
            // Postgres only adds it once, which is fine, but we need to detect such cases.
            if (!bms_is_member(te->ressortgroupref, added_refs)){
                added_refs = bms_add_member(added_refs, te->ressortgroupref);
                subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, copyObject(te));
            }
        }
        subquery->hasWindowFuncs = true;

        WindowDef *window_def = makeNode(WindowDef);
        window_def->orderClause = sort_clause;

        Node *fc_node = traceprov_get_function_call_node(TRACEPROV_ROW_NUMBER, NIL, window_def);
        if (!IsA(fc_node, WindowFunc))
            elog(ERROR, "Expected the row_number call to be function call!");

        if (window_def->orderClause == NIL)
            elog(ERROR, "Got the order clause corrupted!");


        WindowClause *window_clause = makeNode(WindowClause);
        window_clause->orderClause = window_def->orderClause;
        subquery->windowClause = lappend(subquery->windowClause, window_clause);
        window_clause->winref = list_length(subquery->windowClause);
        set_winref(fc_node, list_length(subquery->windowClause));
        TargetEntry *ordered_row_number_te = makeTargetEntry((Expr *)fc_node, 0, pstrdup("ordered_row_number"), false);
        subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, ordered_row_number_te);
        // This ends up being used in both rows and range/groups case.
        Var *ordered_row_number_var = makeVarFromTargetEntry(1, ordered_row_number_te);
        Node *frame_start_fc_node, *frame_end_fc_node;
        
        if (wc->frameOptions & FRAMEOPTION_ROWS){
            // Extend the order clause.
            // The inner target entry doesn't need to be made a var otherwise.
            TargetEntry *outer_ordered_row_number_te = makeTargetEntry((Expr*)ordered_row_number_var, 0, pstrdup("projection_ordered_row_number"), false);
            query->targetList = traceprov_append_at_resjunk(query->targetList, outer_ordered_row_number_te);
            SortGroupClause *ordered_row_number_sgc = makeSortGroupClauseForSetOp(exprType((Node*)outer_ordered_row_number_te->expr), false);
            ordered_row_number_sgc->tleSortGroupRef = assignSortGroupRef(outer_ordered_row_number_te, query->targetList);
            wc->orderClause = lappend(wc->orderClause, ordered_row_number_sgc);

            WindowDef *rows_window_def = makeNode(WindowDef);

            frame_start_fc_node = traceprov_get_function_call_node(TRACEPROV_FIRST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
            frame_end_fc_node = traceprov_get_function_call_node(TRACEPROV_LAST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
        }else {
            // This is range or groups.
            WindowDef *rows_window_def = makeNode(WindowDef);

            frame_start_fc_node = traceprov_get_function_call_node(TRACEPROV_MIN_VALUE, list_make1(ordered_row_number_var), rows_window_def);
            frame_end_fc_node = traceprov_get_function_call_node(TRACEPROV_MAX_VALUE, list_make1(ordered_row_number_var), rows_window_def);
        }

        set_winref(frame_start_fc_node, window_clause->winref);
        set_winref(frame_end_fc_node, window_clause->winref);

        query->targetList = traceprov_append_at_resjunk(query->targetList, makeTargetEntry((Expr *)frame_start_fc_node, 0, pstrdup("frame_start"), false));
        query->targetList = traceprov_append_at_resjunk(query->targetList, makeTargetEntry((Expr *)frame_end_fc_node, 0, pstrdup("frame_end"), false));
    }

    // Finally nest the inner query.
    RangeTblEntry *rte = range_table_entry_from_subquery(subquery, context, true);
    query->rtable = list_make1(rte);
    query->jointree = traceprov_make_from_expr(rte);
    return query;
}