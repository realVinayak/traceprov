#include "rewriter_utils.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_clause.h"
#include "parser/analyze.h"
#include "optimizer/optimizer.h"

static Query *perform_window_clause_rewrite_inline(
    Query *query,
    List *window_clauses,
    TraceProvParseContext *context,
    List *traceprov_provenance_attrs,
    List **extra_targets
);

static TargetEntry *append_ordered_row_number(Query *query, Query *subquery, List *sort_clause, Bitmapset **added_refs);

static bool is_empty_window_frame(const WindowClause *wc, int64 *start_offset, int64 *end_offset);
static int64 assert_int8_const(const Node *result);

// Technically, rows can be changed with group / range since row numbers are guaranteed to be unique.
#define TRACEPROV_FRAMEOPTION_ORDERED_ROW_NUMBERS (FRAMEOPTION_ROWS | FRAMEOPTION_START_UNBOUNDED_PRECEDING | FRAMEOPTION_END_CURRENT_ROW)

// main entrypoint for window rewriting.
// Postgres already sets up important things for us
// (like copying window defs, sort group clauses, setting up winrefs.)
// So, this is not _that_ much pain...
Query *traceprov_perform_window_rewrite(
    Query *base,
    TraceProvParseContext *context,
    List *traceprov_targets,
    List **extra_targets
){
    if (!base->hasWindowFuncs)
        elog(ERROR, "Expected window functions to be present for rewriting traceprov!");
    Query *top_query = perform_window_clause_rewrite_inline(base, list_copy(base->windowClause), context, traceprov_targets, extra_targets);
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
    TraceProvParseContext *context,
    List *traceprov_provenance_attrs,
    List **extra_targets
){
    // Push down the current query.
    // This is needed because, oth
    List *shift_spec = NIL;
    Query *subquery = traceprov_push_down_query(query, &shift_spec);

    // Adjust the current level vars to point to RTE 1.
    // This is done before we append new targets.
    List *current_vars = pull_vars_of_level((Node*)query, 0);

    // Also adjust all the var refs in traceprov targets too.
    List *traceprov_targets_flattened = traceprov_flatten(traceprov_provenance_attrs);
    List *traceprov_exprns_flattened = traceprov_append_targets(traceprov_targets_flattened, NIL);
    List *traceprov_vars = traceprov_assert_all_vars(traceprov_exprns_flattened);
    current_vars = list_concat_copy(current_vars, traceprov_vars);
    ListCell *var_cursor = NULL;
    foreach(var_cursor, current_vars){
        Var *var = lfirst_node(Var, var_cursor);
        var->varattno += list_nth_int(shift_spec, var->varno - 1);
        var->varno = 1;
        var->varattnosyn = var->varattno;
        var->varnosyn = var->varno;
    }

    // Need to push down all the targets that are referenced in the window clauses.
    ListCell *window_clause_cursor = NULL;

    Bitmapset *added_refs = NULL;

    // First, we try figuring out just 1 window clause that an sorted order.
    // We process that window clause first, and then proceed the rest. The process order doesn't otherwise matter.
    // This is done so that we don't have to do explicit logging for all the empty () clauses. They get "stickied"
    // to the first over(non-empty clause). This is because all the logging is the same. Any total logging is fine.
    int window_clause_idx = 0;
    foreach(window_clause_cursor, window_clauses){
        WindowClause *wc = lfirst_node(WindowClause, window_clause_cursor);
        if (list_length(wc->orderClause) > 0 || list_length(wc->partitionClause) > 0) break;
        window_clause_idx++;
    }

    if (window_clause_cursor != NULL){
        WindowClause *wc = lfirst_node(WindowClause, window_clause_cursor);
        // Move the found clause to the first of the list.
        window_clauses = list_delete_nth_cell(window_clauses, window_clause_idx);
        window_clauses = lcons(wc, window_clauses);
    }

    TraceProvLayerNumber first_log = 0;

    List *added_traceprov_targets = NIL;
    bool seen_empty_frame = false;
    foreach(window_clause_cursor, window_clauses){
        WindowClause *wc = lfirst_node(WindowClause, window_clause_cursor);

        // In the case where frame is empty, still need to emit the current row.
        // However, without looking at the data, we cannot possibly figure out all cases
        // where the frames are empty. So, we might overlog in those rare cases.
        // However, in cases where we can provably guarantee that the frames are completely empty,
        // we add the current provenance attributes to the target list.
        int64 start_offset, end_offset;
        if (is_empty_window_frame(wc, &start_offset, &end_offset)){
            // Provably empty frame case.
            // Simply append the current provenance attrs.
            // We should not keep on adding raw targets if they've been added before.
            if (!seen_empty_frame){
                seen_empty_frame = true;
                added_traceprov_targets = list_concat(added_traceprov_targets, traceprov_targets_flattened);
            }
            continue;
        }

        List *sort_clause = list_concat_copy(
            list_copy_deep(wc->partitionClause),
            list_copy_deep(wc->orderClause)
        );

        if (list_length(sort_clause) > 0){

            TargetEntry *ordered_row_number_te = append_ordered_row_number(query, subquery, sort_clause, &added_refs);
            // This ends up being used in both rows and range/groups case.
            Var *ordered_row_number_var = makeVarFromTargetEntry(1, ordered_row_number_te);
            Node *frame_start_fc_node, *frame_end_fc_node;

            TargetEntry *outer_ordered_row_number_te = makeTargetEntry((Expr*)ordered_row_number_var, 0, pstrdup("projection_ordered_row_number"), false);
            outer_ordered_row_number_te->resjunk = true;
            query->targetList = traceprov_append_at_resjunk(query->targetList, outer_ordered_row_number_te);
            SortGroupClause *ordered_row_number_sgc = makeSortGroupClauseForSetOp(exprType((Node*)outer_ordered_row_number_te->expr), false);
            ordered_row_number_sgc->tleSortGroupRef = assignSortGroupRef(outer_ordered_row_number_te, query->targetList);

            if (wc->frameOptions & FRAMEOPTION_ROWS){

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

            WindowClause *marker_window_clause = NULL;
            // In cases where the frame doesn't include the current row,
            // need to extend the window clause to include it.
            if (start_offset > 0 || end_offset < 0){
                marker_window_clause = copyObject(wc);
                if (start_offset > 0){
                    // Need to start the frame at the current row.
                    marker_window_clause->frameOptions &= ~FRAMEOPTION_START_OFFSET_FOLLOWING;
                    marker_window_clause->frameOptions |= FRAMEOPTION_START_CURRENT_ROW;
                    marker_window_clause->startOffset = NULL;
                }else{
                    // Need to end the frame at the current row.
                    marker_window_clause->frameOptions &= ~FRAMEOPTION_END_OFFSET_PRECEDING;
                    marker_window_clause->frameOptions |= FRAMEOPTION_END_CURRENT_ROW;
                    marker_window_clause->endOffset = NULL;
                }
                query->windowClause = lappend(query->windowClause, marker_window_clause);
                marker_window_clause->winref = list_length(query->windowClause);
            }else{
                marker_window_clause = wc;
            }
            set_winref(frame_start_fc_node, marker_window_clause->winref);
            set_winref(frame_end_fc_node, marker_window_clause->winref);

            WindowDef *rows_window_def = makeNode(WindowDef);
            List *aggregate_target = NIL;
            Node *window_fc_node = NULL;
            TraceProvLayerNumber layer_number = traceprov_aggregate_rewrite(
                traceprov_targets_flattened,
                &aggregate_target,
                context,
                true,
                rows_window_def,
                &window_fc_node,
                true
            );
            if (window_fc_node == NULL) elog(ERROR, "Expected log window fc to be set!");
            // Need to set the window clause.
            WindowClause *ordered_row_number_window_clause = makeNode(WindowClause);
            ordered_row_number_window_clause->orderClause = list_make1(ordered_row_number_sgc);
            ordered_row_number_window_clause->frameOptions = TRACEPROV_FRAMEOPTION_ORDERED_ROW_NUMBERS;
            query->windowClause = lappend(query->windowClause, ordered_row_number_window_clause);
            ordered_row_number_window_clause->winref = list_length(query->windowClause);
            set_winref(window_fc_node, ordered_row_number_window_clause->winref);

            if (first_log == 0){
                // The same log ordering is reused when dealing with empty over() clauses, so that relogging is not needed.
                first_log = layer_number;
            }

            added_traceprov_targets = list_concat(
                added_traceprov_targets,
                aggregate_target
            );

            added_traceprov_targets = lappend(
                added_traceprov_targets,
                makeTraceProvTarget(
                    false,
                    makeTargetEntry((Expr *)frame_start_fc_node, 0, pstrdup("frame_start"), false),
                    NULL,
                    0,
                    false,
                    NIL,
                    traceprov_make_window_frame_entry(TP_ENTRY_FRAME_START, layer_number),
                    false
                )
            );

            added_traceprov_targets = lappend(
                added_traceprov_targets,
                makeTraceProvTarget(
                    false,
                    makeTargetEntry((Expr *)frame_end_fc_node, 0, pstrdup("frame_end"), false),
                    NULL,
                    0,
                    false,
                    NIL,
                    traceprov_make_window_frame_entry(TP_ENTRY_FRAME_END, layer_number),
                    false
                )
            );

        }else{
            // This is the case where there's an empty over () clause.
            // We may get lucky and see an existing logged clause.
            // In that case, use it.
            // Otherwise, create a fresh log.
            if (first_log == 0){
                // At this point, we're technically also guaranteed that the current window clause is also empty.
                // So we also use it (rather than creating another window clause.)
                WindowDef *rows_window_def = makeNode(WindowDef);
                List *aggregate_target = NIL;
                Node *window_fc_node = NULL;
                TraceProvLayerNumber layer_number = traceprov_aggregate_rewrite(
                    traceprov_targets_flattened,
                    &aggregate_target,
                    context,
                    true,
                    rows_window_def,
                    &window_fc_node,
                    // This is marked false for a reason.
                    // Doing it this way will allow the window to be derived later on, automatically.
                    // Since we'll only see once of this, it's fine.
                    false
                );
                if (window_fc_node == NULL) elog(ERROR, "Expected log window fc to be set!");
                set_winref(window_fc_node, wc->winref);
                added_traceprov_targets = list_concat(
                    added_traceprov_targets,
                    aggregate_target
                );
                first_log = layer_number;
            }
            added_traceprov_targets = lappend(
                added_traceprov_targets,
                makeTraceProvTarget(
                    false,
                    makeTargetEntry((Expr *)makeInt8Const(first_log), 0, pstrdup("frame_inherit"), false),
                    NULL,
                    0,
                    false,
                    NIL,
                    traceprov_make_window_frame_entry(TP_ENTRY_FRAME_INHERIT, first_log),
                    false
                )
            );
        }
    }

    // Finally nest the inner query.
    RangeTblEntry *rte = range_table_entry_from_subquery(subquery, context, true);
    query->rtable = list_make1(rte);
    query->jointree = traceprov_make_from_expr(rte);
    // rte->eref->colnames
    *extra_targets = added_traceprov_targets;
    return query;
}

static TargetEntry *append_ordered_row_number(Query *query, Query *subquery, List *sort_clause, Bitmapset **added_refs){
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
        if (!bms_is_member(te->ressortgroupref, *added_refs)){
            *added_refs = bms_add_member(*added_refs, te->ressortgroupref);
            subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, copyObject(te));
        }
    }

    WindowDef *window_def = makeNode(WindowDef);
    window_def->orderClause = sort_clause;

    Node *fc_node = traceprov_get_function_call_node(TRACEPROV_ROW_NUMBER, NIL, window_def);
    if (window_def->orderClause == NIL)
        elog(ERROR, "Got the order clause corrupted!");

    WindowClause *window_clause = makeNode(WindowClause);
    window_clause->orderClause = window_def->orderClause;
    subquery->windowClause = lappend(subquery->windowClause, window_clause);
    window_clause->winref = list_length(subquery->windowClause);
    set_winref(fc_node, list_length(subquery->windowClause));
    TargetEntry *ordered_row_number_te = makeTargetEntry((Expr *)fc_node, 0, pstrdup("ordered_row_number"), false);
    subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, ordered_row_number_te);
    subquery->hasWindowFuncs = true;
    return ordered_row_number_te;
}

static int64 assert_int8_const(const Node *result){
    if (!IsA(result, Const)) elog(ERROR, "Expected node to be a constant!");
    const Const *result_const = (Const *)result;
    if (result_const->consttype != INT8OID) elog(ERROR, "Expected the result to be of type int8!");
    return DatumGetInt64(result_const->constvalue);
}

// Checks whether the window frames are always going to be empty.
// In non-offset case, it can always be non-empty. In those cases,
// it'll be empty only in the case where the number of input rows is 0.
static bool is_empty_window_frame(const WindowClause *wc, int64 *start_offset, int64 *end_offset){
    *start_offset = 0;
    *end_offset = 0;
    const int preceding_mask = (FRAMEOPTION_START_OFFSET_PRECEDING | FRAMEOPTION_END_OFFSET_PRECEDING);
    const int following_mask = (FRAMEOPTION_START_OFFSET_FOLLOWING | FRAMEOPTION_END_OFFSET_FOLLOWING);

    if (!(wc->frameOptions & preceding_mask) && !(wc->frameOptions & following_mask)) return false;

    if ((wc->frameOptions & FRAMEOPTION_RANGE)){
        *start_offset = (wc->frameOptions & following_mask) ?  1 : 0;
        *end_offset = (wc->frameOptions & preceding_mask) ? -1 : 0;
        return false;
    }

    if (wc->startOffset == NULL || wc->endOffset == NULL) elog(ERROR, "Expected both the expressions to be set!");

    const Node *start_result = eval_const_expressions(NULL, (Node *) wc->startOffset);
    *start_offset = assert_int8_const(start_result) * ((wc->frameOptions & FRAMEOPTION_START_OFFSET_FOLLOWING) ? 1 : -1);

    const Node *end_result = eval_const_expressions(NULL, (Node *) wc->endOffset);
    *end_offset = assert_int8_const(end_result) * ((wc->frameOptions & FRAMEOPTION_END_OFFSET_FOLLOWING) ? 1 : -1);

    // Both being equal to 0 is a special case.
    // In that case, the frame will always include just the current row.
    // So we can always treat it as empty frame.
    if (*start_offset > *end_offset || (*start_offset == 0 && *end_offset == 0)) return true;
    return false;
}
