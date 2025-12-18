#include "rewriter_utils.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_clause.h"
#include "parser/analyze.h"
#include "optimizer.h"

static Query *perform_window_clause_rewrite(Query *base, WindowClause *window_clause, List **extra_targets, TraceProvParseContext *context);
static List *remove_window_clause(List *original_clauses, const WindowClause *window_clause);
static Query *perform_window_clause_rewrite_recursive(
    Query *query,
    List *window_clauses,
    TraceProvParseContext *context
);

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

    // // const List *original_sort_clause = list_copy_deep(base->sortClause);
    // Query *top_query = NULL;
    // Query *last_query = NULL;
    // // Need to make a copy of the original window clause since we'll mutate it.
    // int max_ref_seen = -1;
    // List *window_clauses_to_remove = list_copy(base->windowClause);
    // for (int idx = list_length(window_clauses_to_remove) - 1; idx >= 0; idx--){
    //     WindowClause *wc = (WindowClause *)list_nth(window_clauses_to_remove, idx);
    //     if (max_ref_seen == -1){
    //         max_ref_seen = wc->winref;
    //     }else{
    //         if (max_ref_seen <= wc->winref){
    //             elog(ERROR, "Trying to remove ref: %d again!", wc->winref);
    //         }
    //     }
    //     // We always modify the base query (it keeps on getting nested till there are no window clauses are present.)
    //     Query *created = perform_window_clause_rewrite(base, wc, NULL, context);
    //     if (top_query == NULL){
    //         // first iteration.
    //         top_query = created;
    //     }else if (last_query != NULL){
    //         ((RangeTblEntry *)lfirst(list_head(last_query->rtable)))->subquery = created;
    //     }
    //     last_query = created;
    // }
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
    List *current_vars = pull_vars_of_level(query, 0);
    ListCell *var_cursor = NULL;
    foreach(var_cursor, current_vars){
        Var *var = lfirst_node(Var, var_cursor);
        var->varattno += list_nth_int(shift_spec, var->varno - 1);
        var->varno = 1;
    }

    // Need to push down all the targets that are referenced in the window clauses.
    List *new_window_clauses = NIL;
    List *new_window_funcs = NIL;
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
            if (bms_is_member(te->ressortgroupref, added_refs)) continue;
            added_refs = bms_add_member(added_refs, te->ressortgroupref);
            subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, copyObject(te));
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
        subquery->windowClause = lappend(subquery->windowClause);
        window_clause->winref = list_length(subquery->windowClause);
        set_winref(fc_node, list_length(subquery->windowClause));
        TargetEntry *ordered_row_number_te = makeTargetEntry((Expr *)fc_node, 0, pstrdup("ordered_row_number"), false);
        subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, ordered_row_number_te);
        
        if (window_clause->frameOptions & FRAMEOPTION_ROWS){
            // Extend the order clause.
            // The inner target entry doesn't need to be made a var otherwise.
            Var *outer_ordered_row_number_te = makeVarFromTargetEntry(1, ordered_row_number_te);
            query->targetList = traceprov_append_at_resjunk(query->targetList, outer_ordered_row_number_te);
            SortGroupClause *ordered_row_number_sgc = makeSortGroupClauseForSetOp(exprType((Node*)outer_ordered_row_number_te->expr), false);
            outer_ordered_row_number_te->ressortgroupref = assignSortGroupRef(outer_ordered_row_number_te, final_query->targetList);
            window_clause->orderClause = lappend(window_clause->orderClause, ordered_row_number_sgc);

            WindowDef *rows_window_def = makeNode(WindowDef);

            Node *first_value_fc_node = traceprov_get_function_call_node(TRACEPROV_FIRST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
            if (!IsA(first_value_fc_node, WindowFunc))
                elog(ERROR, "Expected the row_number call to be function call!");

            Node *last_value_fc_node = traceprov_get_function_call_node(TRACEPROV_LAST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
            if (!IsA(last_value_fc_node, WindowFunc))
                elog(ERROR, "Expected the row_number call to be function call!");
            
            // It'll be always be 1
            set_winref(first_value_fc_node, window_clause->winref);
            set_winref(last_value_fc_node, window_clause->winref);

            final_query->targetList = traceprov_append_at_resjunk(final_query->targetList, makeTargetEntry((Expr *)first_value_fc_node, 0, pstrdup("frame_start"), false));
            final_query->targetList = traceprov_append_at_resjunk(final_query->targetList, makeTargetEntry((Expr *)last_value_fc_node, 0, pstrdup("frame_end"), false));
        }else {
            // TODO: Handle group and range case.
            elog(ERROR, "Only support groups and range for now.");
        }
    }

    // Finally nest the inner query.
    query->rtable = list_make1(range_table_entry_from_subquery(subquery, context, true));
    return query;
}

static Query *perform_window_clause_rewrite_recursive(
    Query *query,
    List *window_clauses,
    TraceProvParseContext *context
){
    // This is the base case (no window clauses found.)
    if (list_length(window_clauses) == 0){
        query->hasWindowFuncs = false;
        query->windowClause = NIL;
        return query;
    }

    // We remove from the tail. It actually doesn't matter, but removing from the tail is cheaper.
    WindowClause *window_clause_to_remove = (WindowClause *)lfirst(list_tail(window_clauses));
    List *remaining_window_clauses = list_delete_last(window_clauses);

    // The window functions that corresponding to this window clause.
    List *window_funcs_target_list = NIL;
    List *window_funcs_position = NIL;
    ListCell *target_entry_cursor = NULL;

    
    foreach(target_entry_cursor, query->targetList){
        TargetEntry *te = lfirst(target_entry_cursor);
        if (IsA(te->expr, WindowFunc)){
            WindowFunc *window_func = (WindowFunc *)(te->expr);
            if (window_func->winref == window_clause_to_remove->winref){
                // Setting it to 1 here makes handling things later on simpler,
                // since they don't have to be altered later.
                window_func->winref = 1;
                TargetEntry *old_te = copyObject(te);
                window_funcs_target_list = lappend(window_funcs_target_list, old_te);
                window_funcs_position = lappend_int(window_funcs_position, foreach_current_index(target_entry_cursor));
                te->expr = (Expr*)makeNullConst(
                    exprType((Node *)old_te->expr),
                    exprTypmod((Node *)old_te->expr),
                    exprCollation((Node *)old_te->expr)
                );
            }
        }
        // So that the columns that refer to are still valid (in the top level queries.)
        te->resjunk = false;
    }

    // Rewrite the next window clause. This _could_ return the same query back (and that is fine.)
    Query *child_rewritten = perform_window_clause_rewrite_recursive(
        query,
        remaining_window_clauses,
        context
    );

    // Nest the child query.
    Query *nested = traceprov_make_nested_query(child_rewritten, context, true, true);

    // Add the row_number() to the nested. This way, there's only one window clause per query block.
    List *sort_clause = list_concat_copy(
        list_copy_deep(window_clause_to_remove->partitionClause),
        list_copy_deep(window_clause_to_remove->orderClause)
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
    window_clause->winref = 1;
    WindowFunc *window_fc_node = (WindowFunc *) fc_node;
    window_fc_node->winref = window_clause->winref;
    // Note that the nested won't have any window clause (so, need to also set that up)
    nested->hasWindowFuncs = true;
    // Each query block has just 1 window clause. Basically, this is poor man's loop unrolling.
    nested->windowClause = list_make1(window_clause);
    // Shouldn't see any resjunk in the nested's target list.
    traceprov_assert_no_resjunk(nested->targetList);
    TargetEntry *ordered_row_number_te = makeTargetEntry((Expr *)window_fc_node, 0, pstrdup("ordered_row_number"), false);
    nested->targetList = traceprov_append_at_resjunk(
        nested->targetList,
        ordered_row_number_te
    );
    // Need to nest the query yet again.
    // This is the query where we'll add the current window clause.
    Query *final_query = traceprov_make_nested_query(nested, context, true, true);
    final_query->hasWindowFuncs = true;
    window_clause_to_remove->winref = 1;
    if (list_length(final_query->windowClause) > 0) elog(ERROR, "Expected the query to not have any window clauses!");
    final_query->windowClause = list_make1(window_clause_to_remove);

    TargetEntry *outer_ordered_row_number_te = list_nth(final_query->targetList, ordered_row_number_te->resno - 1);
    outer_ordered_row_number_te->ressortgroupref = assignSortGroupRef(outer_ordered_row_number_te, final_query->targetList);
    SortGroupClause *ordered_row_number_sgc = makeSortGroupClauseForSetOp(exprType((Node*)outer_ordered_row_number_te->expr), false);
    ordered_row_number_sgc->tleSortGroupRef = outer_ordered_row_number_te->ressortgroupref;

    if (window_clause_to_remove->frameOptions & FRAMEOPTION_ROWS){
        window_clause_to_remove->orderClause = lappend(window_clause_to_remove->orderClause, ordered_row_number_sgc);
        WindowDef *rows_window_def = makeNode(WindowDef);

        Node *first_value_fc_node = traceprov_get_function_call_node(TRACEPROV_FIRST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
        if (!IsA(first_value_fc_node, WindowFunc))
            elog(ERROR, "Expected the row_number call to be function call!");

        Node *last_value_fc_node = traceprov_get_function_call_node(TRACEPROV_LAST_VALUE, list_make1(outer_ordered_row_number_te->expr), rows_window_def);
        if (!IsA(last_value_fc_node, WindowFunc))
            elog(ERROR, "Expected the row_number call to be function call!");
        
        // It'll be always be 1
        set_winref(first_value_fc_node, 1);
        set_winref(last_value_fc_node, 1);

        final_query->targetList = traceprov_append_at_resjunk(final_query->targetList, makeTargetEntry((Expr *)first_value_fc_node, 0, pstrdup("frame_start"), false));
        final_query->targetList = traceprov_append_at_resjunk(final_query->targetList, makeTargetEntry((Expr *)last_value_fc_node, 0, pstrdup("frame_end"), false));
    }else {
        // TODO: Handle group and range case.
        elog(ERROR, "Only support groups and range for now.");
    }

    // Need to go over the vars beloning to this window clause ref and make them actual window funcs.
    target_entry_cursor = NULL;
    ListCell *idx_cursor = NULL;
    ListCell *window_func_cursor = NULL;

    traceprov_assert_equal_length(list_make2(window_funcs_position, window_funcs_target_list));

    // The target list can be longer (or equal in length) than the window funcs, but never shorter.
    if (list_length(final_query->targetList) < list_length(window_funcs_position))
        elog(ERROR, "Expected the target list to be at least as big as the window funcs position.");
    
    for (int idx = 0; idx < list_length(window_funcs_position); idx++){
        int window_func_position = list_nth_int(window_funcs_position, idx);
        TargetEntry *old_te = list_nth_node(TargetEntry, window_funcs_target_list, idx);
        TargetEntry *new_te = list_nth_node(TargetEntry, final_query->targetList, window_func_position);
        // Below invariants shouldn't have changed.
        if (old_te->resno != window_func_position + 1) elog(ERROR, "Expected old te's resno to be window func position + 1");
        if (new_te->resno != window_func_position + 1) elog(ERROR, "Expected new te's resno to be window func position + 1");
        if (!IsA(new_te->expr, Var)){
            elog(ERROR, "Expected the new te's expr to be a var reference!");
        }
        // the winref will automatically have been adjusted so this is fine.
        new_te->expr = old_te->expr;
    }

    return final_query;
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
    // Need to remove the current window clause from the base too.
    List *original_window_clauses = list_copy(base->windowClause);
    base->windowClause = remove_window_clause(base->windowClause, input_window_clause);
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
            if (window_func->winref == input_window_clause->winref && ((Node*)te->expr != fc_node)){
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
                te->expr = (Expr*)makeNullConst(
                    exprType((Node *)old_te->expr),
                    exprTypmod((Node *)old_te->expr),
                    exprCollation((Node *)old_te->expr)
                );
            }
        }
    }

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
    nested->hasWindowFuncs = true;

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
