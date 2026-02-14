#include "c.h"
#include "postgres.h"
#include "optimizer/planner.h"
#include "fmgr.h"
#include "traceprov_parse_context.h"
#include "rewriter_utils.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "parser/analyze.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_coerce.h"

// Defining it like this, because sometimes the pointer to 
// oid is needed.
static const Oid int8OidConst = INT8OID;

static List *get_matchable_attrs(List *rawList, List *ignoreList, int index){
    if (ignoreList == NIL) return rawList;
    if (index >= list_length(ignoreList)){
        elog(ERROR, "given index is out of bounds!");
    }
    List *matchables = NIL;
    List *ignoreForRTE =  (List*)list_nth(ignoreList, index);
    ListCell *outerCursor;
    foreach(outerCursor, rawList){
        TargetEntry *te = (TargetEntry *)lfirst(outerCursor);
        // Don't include resjunk columns for matching.
        if (te->resjunk) continue;
        ListCell *innerCursor = NULL;
        bool found = false;
        foreach(innerCursor, ignoreForRTE){
            TargetEntry *toIgnore = ((TraceProvTarget *)lfirst(innerCursor))->targetEntry;
            found = found || (toIgnore->resno == te->resno);
        }
        if (!found){
            matchables = lappend(matchables, te);
        }
    }
    return matchables;
}

List *traceprov_adjust_union(
    SetOperationStmt *root, 
    List *extraTargets,
    List *queryRteList,
    TraceProvParseContext *context
){
    if (root->op != SETOP_UNION){
        elog(ERROR, "Expected op to be of union!");
    }
    traceprov_assert_equal_length(list_make2(extraTargets, queryRteList));

    // Get the max number of extra targets that were added.
    unsigned int maxTargetListLength = 0;
    ListCell *rteCursor = NULL;
    foreach(rteCursor, queryRteList){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCursor);
        traceprov_assert_is_subquery(rte);
        Query *subquery = rte->subquery;
        traceprov_assert_no_resjunk(subquery->targetList);
        maxTargetListLength = Max(maxTargetListLength, list_length(subquery->targetList));
    }

    rteCursor = NULL;
    ListCell *extraTargetCursor;
    List *newExtraTargets = NIL;

    forboth(rteCursor, queryRteList, extraTargetCursor, extraTargets){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCursor);
        Query *subquery = rte->subquery;
        const unsigned int padding = (maxTargetListLength - list_length(subquery->targetList));
        List *extraTargetsForRte = (List *)lfirst(extraTargetCursor);
        List *newlyCreatedTargets = NIL;
        if (padding > 0){
            elog(INFO, "Needed to perform a padding!");
            // In this case, need to add padding NULLs.
            List *nullList = traceprov_get_null_list(padding, INT8OID, -1, InvalidOid);
            ListCell *nullListCursor;
            foreach(nullListCursor, nullList){
                Expr *nullExpr = (Expr *)lfirst(nullListCursor);
                TargetEntry *newTe = makeTargetEntry(nullExpr, 0, pstrdup("tp_null"), false);
                newlyCreatedTargets = lappend(newlyCreatedTargets, newTe);
            }
        }
        const int setNumber = (tp_parse_get_unique_number(context));
        tp_add_set_padding_item(context, setNumber, padding);
        Expr *subqNumberExpr = (Expr*)makeInt8Const(setNumber);
        TargetEntry *subqNumberTarget = makeTargetEntry(
            subqNumberExpr,
            0,
            pstrdup("tp_setop_nummber"),
            false
        );
        newlyCreatedTargets = lappend(newlyCreatedTargets, subqNumberTarget);
        ListCell *newlyCreatedTargetCursor;
        foreach(newlyCreatedTargetCursor, newlyCreatedTargets){
            TargetEntry *newTe = (TargetEntry *)lfirst(newlyCreatedTargetCursor);
            subquery->targetList = traceprov_append_at_resjunk(subquery->targetList, newTe);
            rte->eref->colnames = lappend(rte->eref->colnames, makeString(pstrdup(newTe->resname)));
            const Var *newVar = makeVarFromTargetEntry(foreach_current_index(rteCursor) + 1, newTe);
            TargetEntry *targetInParent = makeTargetEntry(
                (Expr *)newVar,
                newTe->resno,
                newTe->resname,
                false
            );
            bool isSetPointer = foreach_current_index(newlyCreatedTargetCursor) == list_length(newlyCreatedTargets) - 1;
            extraTargetsForRte = lappend(
                extraTargetsForRte,
                makeTraceProvTarget(
                    false,
                    targetInParent,
                    NULL,
                    setNumber,
                    isSetPointer,
                    // These have trivially no sublinks.
                    NIL,
                    NULL
                )
            );
        }
        ListCell *targetCursor;
        List *extraTargetsTyped = NIL;
        List *traceprovEntries = NIL, *childGraphs = NIL;
        foreach(targetCursor, extraTargetsForRte){
            // Here, also need to cast all into INT8s.
            TraceProvTarget *tpTarget = (TraceProvTarget *)(lfirst(targetCursor));
            TargetEntry *copiedTarget = copyObject(tpTarget->targetEntry);
            Expr *expr = copiedTarget->expr;
            if (IsA(expr, Const)){
                const Const *constExpr = (Const*)expr;
                if (constExpr->consttype != INT8OID || constExpr->constcollid != InvalidOid || constExpr->consttypmod != -1){
                    elog(ERROR, "Expected the const's types to be already aligned.");
                }
            } else if(IsA(expr, Var)){
                Var *varExpr = (Var *)expr;
                if (varExpr->vartype != INT8OID){
                    varExpr->vartype = INT8OID;
                    varExpr->vartypmod = -1;
                    varExpr->varcollid = InvalidOid;
                }
            } else {
                elog(ERROR, "Got unexpected expr (the tp should always be var or const!)");
            }
            TraceProvTarget *typedTpTarget = makeTraceProvTarget(
                    tpTarget->isPointer,
                    copiedTarget,
                    tpTarget->graph,
                    setNumber,
                    tpTarget->isSetPointer,
                    tpTarget->sublinks,
                    tpTarget->window_entry
                );

            extraTargetsTyped = lappend(
                extraTargetsTyped,
                typedTpTarget
            );
            traceprovEntries = lappend(traceprovEntries, traceprov_resolve_entry(
                tpTarget,
                &childGraphs,
                NULL
            ));
        }
        newExtraTargets = lappend(newExtraTargets, extraTargetsTyped);
        tp_add_set_graph_item(context, setNumber, make_traceprov_dependency(TP_LOG, 0, childGraphs, traceprovEntries));
    }

    List *setOpFlattened = traceprov_find_used_refs((Node*)root, traceProvInclusiveNavigator);
    ListCell *opCursor;
    const unsigned int expectedLength = maxTargetListLength + 1;
    foreach(opCursor, setOpFlattened){
        Node *node = (Node*)lfirst(opCursor);
        if (IsA(node, SetOperationStmt)){
            SetOperationStmt *setOp = (SetOperationStmt *)node;
            int currentLength = traceprov_assert_equal_length(
                list_make3(
                    setOp->colTypes,
                    setOp->colCollations,
                    setOp->colTypmods
                )
            );
            if (currentLength < expectedLength){
                const int padding = expectedLength - currentLength;
                setOp->colTypes = list_concat(
                    setOp->colTypes, 
                    traceprov_dup_oid(INT8OID, padding)
                );
                setOp->colCollations = list_concat(
                    setOp->colCollations,
                    traceprov_dup_oid(InvalidOid, padding)
                );
                setOp->colTypmods = list_concat(
                    setOp->colTypmods,
                    traceprov_dup_int(-1, padding)
                );
            }
        }
    }

    // Here, also return only for the first (in the case of union, all have, now, the same schema!)
    return list_make1(lfirst(list_head(newExtraTargets)));
}

// For excepts, need to perform deduplication before.
// For unions and intersects, deduplication happens after.
// makes the code a bit ugly, but whatever.
// Some interesting observations:
// 1. Need to, first, perform the type casts (going INTO the EXCEPT)
// 2. Need to, second, revert the type casts (going out of the EXCEPT)
// The steps:
// 1. Iterate through the RTE list, add the typecasts (and addition of null terms)
// 2. Convert the query into a subquery, and get rid of typecasts.
// TODO: See if it is possible to fold the typecasts into 1 step.
Query *traceprov_adjust_except(
    // The input query
    Query *inputQuery,
    List *extraTargets,
    TraceProvParseContext *context,
    bool aggregatedInParent,
    // The created extra targets (AFTER the reverse-type cast.)
    List **createdTargets
){
    SetOperationStmt *root = (SetOperationStmt *)(inputQuery->setOperations);
    List *queryRteList = inputQuery->rtable;
    if (root->op != SETOP_EXCEPT){
        elog(ERROR, "Expected op to be of except!");
    }

    Oid tpPointerTypId = InvalidOid;
    int tpPointerTypMod;
    parseTypeString(TRACEPROV_POINTER_TYPE_NAME, &tpPointerTypId, &tpPointerTypMod, false);

    TraceProvUsedRefNavigator leftNavigator = {
        .goLeft = true,
        .goRight = false,
        .includeInternals = false
    };
    traceprov_assert_equal_length(list_make2(extraTargets, queryRteList));
    List *leftMostElement = traceprov_find_used_refs((Node*)root, leftNavigator);
    // There should always be just one left-most element. assert thatn.
    traceprov_assert_equal_length(list_make2(leftMostElement, list_make1(NULL)));
    const RangeTblRef *leftMostRef = (RangeTblRef *)lfirst(list_head(leftMostElement));
    List *allSetElements = traceprov_find_used_refs((Node*)root, traceProvInclusiveNavigator);
    
    ListCell *setElementCursor = NULL;
    foreach(setElementCursor, allSetElements){
        if (IsA(lfirst(setElementCursor), SetOperationStmt)){
            const SetOperationStmt *internalSetOp = (SetOperationStmt *)lfirst(setElementCursor);
            if ((Node*)internalSetOp->larg == (Node*)leftMostRef) break;
        }
    }
    const SetOperationStmt *leftParentSetOp = (SetOperationStmt *)lfirst(setElementCursor);
    if (leftParentSetOp == NULL) elog(ERROR, "Expected the left most element to have a parent!");
    List *extraTargetsForRTE = (List*)list_nth(extraTargets, leftMostRef->rtindex - 1);
    RangeTblEntry *rte = list_nth(queryRteList, (leftMostRef->rtindex - 1));
    if (!root->all){
        // Need to perform deduplication on the left-most ref.
        // The targets in the base query have not been adjusted yet, so this does simplify some of that.
        Query *modified = traceprov_make_nested_query(rte->subquery, context, false, false);
        // the returned aggregated becomes the new set.
        List *newTargetList = NIL;
        for (int i = 0; i < list_length(modified->targetList); i++){
            if (i < (list_length(modified->targetList) - list_length(extraTargetsForRTE))){
                newTargetList = lappend(newTargetList, list_nth(modified->targetList, i));
                continue;
            }
        }
        List *aggregated = traceprov_aggregate_on_set(
            leftParentSetOp,
            modified,
            context,
            aggregatedInParent,
            newTargetList,
            extraTargetsForRTE
        );
        rte->subquery = modified;
        extraTargetsForRTE = aggregated;
    }else{
        Query *modified = traceprov_make_nested_query(rte->subquery, context, false, false);
        rte->subquery = modified;
        const int originalLength = list_length(modified->targetList) - list_length(extraTargetsForRTE);
        for (int i = 0; i < (list_length(extraTargetsForRTE)); i++){
            ((TraceProvTarget *)(list_nth(extraTargetsForRTE, i)))->targetEntry = list_nth(modified->targetList, i + originalLength);
        }
    }
    const int finalLength = list_length(rte->subquery->targetList);
    // Need to cast the extraTargets to be of the new type.
    ListCell *extraTargetCursor;
    foreach(extraTargetCursor, extraTargetsForRTE){
        TargetEntry *te = ((TraceProvTarget *)lfirst(extraTargetCursor))->targetEntry;
        Oid currentExprType = exprType((Node*)te->expr);
        if (currentExprType != INT8OID){
            // First, cast this into a bigint.
            // This can happen, if, for example, the primary keys are not int8.
            // TODO: Be more cleverer, and don't run into this problem in the first place,
            // by casting the leafs into bigint.
            if (can_coerce_type(1, &currentExprType, &int8OidConst, COERCION_IMPLICIT) == false){
                elog(INFO, "Cannot cast the current expr to int8. This should never happen.: %d", currentExprType);
            }
            te->expr = (Expr*)coerce_type(
                NULL,
                (Node*)te->expr,
                currentExprType,
                INT8OID,
                -1,
                COERCION_IMPLICIT,
                COERCE_IMPLICIT_CAST,
                -1
            );
            if (te->expr->type == T_Invalid){
                elog(ERROR, "Got error coercing type!");
            }
            currentExprType = INT8OID;
        }
        // Now, need to cast the Int8 -> traceprov_ptr_type
        if (can_coerce_type(1, &currentExprType, &tpPointerTypId, COERCION_IMPLICIT) == false){
            elog(ERROR, "Should be able to cast Int8 -> traceprov_ptr_type!");
        }
        te->expr = (Expr*)coerce_type(
            NULL,
            (Node*)te->expr,
            currentExprType,
            tpPointerTypId,
            -1,
            COERCION_IMPLICIT,
            COERCE_IMPLICIT_CAST,
            -1
        );
        if (te->expr->type == T_Invalid){
            elog(ERROR, "Got error coercing type!");
        }
    }

    const int padding = list_length(extraTargetsForRTE);
    for (int extraTargetsCursor = 0; extraTargetsCursor < list_length(extraTargets); extraTargetsCursor++){
        if ((extraTargetsCursor) != (leftMostRef->rtindex - 1)){
            // In this case, also need to expand out with constants.
            // And, then, need to expand out the subqueries with that constant too.
            RangeTblEntry *rte = list_nth(queryRteList, (extraTargetsCursor));
            traceprov_assert_is_subquery(rte);
            const int currentLength = list_length(rte->subquery->targetList);
            if (currentLength >= finalLength){
                elog(ERROR, "expected the initial excepts to always be underfiled!");
            }
            if (padding != (finalLength - currentLength)){
                elog(ERROR, "Expected padding to be same as the diff of finaltarget list and current length!");
            }
            for (int i = 0; i < finalLength - currentLength; i++){
                Expr *expr =(Expr*)makeConst(tpPointerTypId, -1, InvalidOid, sizeof(int64), 0, false, true);
                TargetEntry *targetEntry = makeTargetEntry(
                    expr,
                    0,
                    pstrdup("tp_except_proxy"),
                    false
                );
                rte->subquery->targetList = traceprov_append_at_resjunk(rte->subquery->targetList, targetEntry);
                rte->eref->colnames = lappend(rte->eref->colnames, makeString(pstrdup(targetEntry->resname)));
            }
        }
    }
    
    setElementCursor = NULL;
    foreach(setElementCursor, allSetElements){
        if (IsA(lfirst(setElementCursor), SetOperationStmt)){
            // Need to extend the number of elements, in this, depending on the length of extra targets.
            SetOperationStmt *internalSetOp = (SetOperationStmt *)lfirst(setElementCursor);
            if (list_length(internalSetOp->groupClauses) == 0){
                elog(ERROR, "Expected some group clauses to be filled!");
            }
            internalSetOp->colCollations = list_concat(
                internalSetOp->colCollations,
                traceprov_dup_oid(InvalidOid, padding)
            );
            internalSetOp->colTypmods = list_concat(
                internalSetOp->colTypmods,
                traceprov_dup_int(-1, padding)
            );
            internalSetOp->colTypes = list_concat(
                internalSetOp->colTypes,
                traceprov_dup_oid(tpPointerTypId, padding)
            );
            for (int i = 0; i < padding; i++){
                internalSetOp->groupClauses = lappend(
                    internalSetOp->groupClauses,
                    makeSortGroupClauseForSetOp(tpPointerTypId, false)
                );
            }
        }
    }
    // Need to go over the extra targets, and append them to current query block.
    // Then, need to make it a subquery, and in the outer query, add the casts back.
    inputQuery->targetList = traceprov_append_targets(extraTargetsForRTE, inputQuery->targetList);
    // Now, need to wrap the input query inside a subquery.
    // Get the new child targets.
    List *childTargets = traceprov_propagate_child_targets(extraTargetsForRTE, 1);
    Query *parent = traceprov_make_nested_query(inputQuery, context, false, false);
    ListCell *childTargetCursor;
    foreach(childTargetCursor, childTargets){
        TraceProvTarget *tpTarget = (TraceProvTarget *)lfirst(childTargetCursor);
        Oid currentType = exprType((Node*)tpTarget->targetEntry->expr);
        if (currentType != tpPointerTypId){
            elog(
                ERROR, 
                "Expected the type of child created in except to be of type traceprov_ptr. But got: %d",
                currentType
            );
        }
        if (can_coerce_type(1, &currentType, &int8OidConst, COERCION_IMPLICIT) == false){
            elog(INFO, "Expected traceprov_ptr_type to be castable back to int8!");
        }
        tpTarget->targetEntry->expr = (Expr*)coerce_type(
            NULL,
            (Node*)tpTarget->targetEntry->expr,
            currentType,
            int8OidConst,
            -1,
            COERCION_IMPLICIT,
            COERCE_IMPLICIT_CAST,
            -1
        );
        if (tpTarget->targetEntry->expr->type == T_Invalid){
            elog(ERROR, "Got error coercing type!");
        }
    }
    // For the new child targets, to the cast back from traceprov_ptr_type -> bigint.
    *createdTargets = childTargets;
    return parent;
}

List *traceprov_adjust_intersect(Query *base, List *ignore_list, TraceProvParseContext *context){
    // Here, need to replace the intersect with a join.
    // For each table pair present, need to construct the join, and the join tree.
    // Basically, all the intersects within this block are consumed into a join.
    // It doesn't matter what join order is used (optimizer might reorder it anyways)
    // Simplest order is the one in RTElist, so it is directly used.
    // However, need to ignore the traceprov attributes in the join condition.
    // So, the ignoreList, if given, is consulted. If the table index AND column idx is same,
    // column is ignored from predicates.
    const List *used_references = traceprov_find_used_refs(base->setOperations, traceProvLeafNavigator);
    if (list_length(used_references) != list_length(base->rtable)){
        elog(ERROR, "Expected the used references to be of the same size as setOperations");
    }
    if (list_length(used_references) < 2){
        elog(ERROR, "Expected at least 2 elements for the join!");
    }
    // If we're an ALL, in that case, need to use window function formulation to only
    // get the minimum from both side.
    const Node *base_setop_node = base->setOperations;
    List *extra_targets_per_rte = ignore_list;
    if (!IsA(base_setop_node, SetOperationStmt)){
        elog(ERROR, "Expected base to be a setop node!");
    }else{
        const SetOperationStmt *base_setop = (SetOperationStmt *)base_setop_node;
        // Need to use window function formulation.
        // Here, need to also level-down the current subqueries.
        // We don't need to touch the set operations itself, since it'll be removed.
        // Here, need to also add the aggregation over the traceprov attributes (window function)
        // Also, the traceprov aggregates aren't removed. Actually, during optimization time, they'll be removed, since
        // they aren't needed at any place. This simplifies the rewriting, since the added attributes don't have to be removed lol.
        // HOWEVER, the added aggregation should not be joined (since it could be different)
        if (base_setop->all){
            ListCell *rte_cursor;
            List *extra_targets_per_rte_all = NIL;
            foreach(rte_cursor, base->rtable){
                RangeTblEntry *rte = (RangeTblEntry *)lfirst(rte_cursor);
		        traceprov_assert_is_subquery(rte);
                Query *subquery = rte->subquery;
                Query *cloned_subquery = traceprov_make_nested_query(subquery, context, false, false);
               	rte->subquery = cloned_subquery;
                List *target_entries = get_matchable_attrs(cloned_subquery->targetList, ignore_list, foreach_current_index(rte_cursor));
                // Make the WindowDef. This is done so that logic in ParseFuncOrColumn can be reused.
                WindowDef *window_def = makeNode(WindowDef);
                window_def->partitionClause = NIL;
                ListCell *target_entry_cursor = NULL;
                foreach(target_entry_cursor, target_entries){
                    TargetEntry *te = (TargetEntry *)lfirst(target_entry_cursor);
                    SortGroupClause *sort_group_clause = makeSortGroupClauseForSetOp(exprType((Node*)te->expr), false);
                    window_def->partitionClause = lappend(window_def->partitionClause, sort_group_clause);
                    sort_group_clause->tleSortGroupRef = list_length(window_def->partitionClause);
                    if (te->ressortgroupref != 0)
                        elog(ERROR, "Attempting to overwrite ressortgroupref!");
                    te->ressortgroupref = sort_group_clause->tleSortGroupRef;
                }
                Node *fc_node = traceprov_get_function_call_node(TRACEPROV_ROW_NUMBER, NIL, window_def);
                if (!IsA(fc_node, WindowFunc)){
                    elog(ERROR, "Expected the row_number call to be function call!");
                }
                // Here, it is actually impossible to share the ref with anyone else, since the added window ref
                // sits on top of it (so, no possibility of sharing any windeo defs)
                WindowClause *window_clause = makeNode(WindowClause);

                if (window_def->partitionClause == NIL)
                    elog(ERROR, "Got the partition clause corrupted!");

                window_clause->partitionClause = window_def->partitionClause;
                window_clause->frameOptions = FRAMEOPTION_DEFAULTS;

                if (cloned_subquery->windowClause != NIL)
                    elog(ERROR, "Expected the clone subquery to have no window clauses!");

                cloned_subquery->windowClause = lappend(cloned_subquery->windowClause, window_clause);
                window_clause->winref = list_length(cloned_subquery->windowClause);
                WindowFunc *window_fc_node = (WindowFunc *)fc_node;
                window_fc_node->winref = window_clause->winref;
                cloned_subquery->hasWindowFuncs = true;
                char *window_target_name = tp_parse_get_unique_alias(context);
		        rte->eref->colnames = lappend(rte->eref->colnames, makeString(window_target_name));
		        cloned_subquery->targetList = traceprov_append_at_resjunk(
                    cloned_subquery->targetList,
                    makeTargetEntry((Expr *)window_fc_node, 0, window_target_name, false)
                );
                List *traceprov_aggregated_window = NIL;
                List *traceprov_subquery_targets = traceprov_propagate_child_targets(
                    list_nth(ignore_list, foreach_current_index(rte_cursor)),
                    1
                );
                traceprov_aggregate_rewrite(
                    traceprov_subquery_targets,
                    &traceprov_aggregated_window,
                    context,
                    true,
                    window_def,
                    NULL
                );
                TargetEntry *traceprov_log_te = ((TraceProvTarget *)(lfirst(list_head(traceprov_aggregated_window))))->targetEntry;
                Node *traceprov_log_node = (Node*)traceprov_log_te->expr;
                if (!IsA(traceprov_log_node, WindowFunc)){
                    elog(ERROR, "Expected the traceprov log here to be a window function!");
                }
                WindowFunc *window_traceprov_log_node = ((WindowFunc *)traceprov_log_node);
                window_traceprov_log_node->winref = window_clause->winref;
                List *current_traceprov_targets = (List *)list_nth(ignore_list, foreach_current_index(rte_cursor));
                current_traceprov_targets = traceprov_append_targets(traceprov_aggregated_window, current_traceprov_targets);
                // Add the newly added traceprov window function to the ignore list (it should be ignored during joining)
                list_nth_cell(ignore_list, foreach_current_index(rte_cursor))->ptr_value = current_traceprov_targets;
                // Add the aggregated window to the subquery's target list.
                cloned_subquery->targetList = traceprov_append_targets(traceprov_aggregated_window, cloned_subquery->targetList);
                rte->eref->colnames = lappend(rte->eref->colnames, makeString(traceprov_log_te->resname));
                // At this point, the target list is consistent.
                // Moreover, the added window function will also be skipped during the 
                extra_targets_per_rte_all = lappend(
                    extra_targets_per_rte_all,
                    traceprov_propagate_child_targets(traceprov_aggregated_window, foreach_current_index(rte_cursor) + 1)
                );
            }
            if (list_length(extra_targets_per_rte_all) != list_length(base->rtable)){
                elog(ERROR, "Got mismatching traceprov window function count!");
            }
            extra_targets_per_rte = extra_targets_per_rte_all;
        }
    }

    RangeTblEntry *first_rte = (RangeTblEntry*) lfirst(list_head(base->rtable));
    traceprov_assert_is_subquery(first_rte);
    first_rte->inFromCl = true;
    List *addedJoins = NIL;
    ListCell *rteCursor;
    RangeTblEntry *last_join_entry = NULL;
    JoinExpr *join_expr = NULL;

    // Start from the next table.
    int i = 1;
    for_each_from(rteCursor, base->rtable, 1){
        i++;
        RangeTblEntry *next_rte = (RangeTblEntry *)lfirst(rteCursor);
        traceprov_assert_is_subquery(next_rte);
        next_rte->inFromCl = true;
        List *col_names = NIL;
        ListCell *target_var_cursor;
        if (last_join_entry == NULL){
            last_join_entry = makeNode(RangeTblEntry);
            last_join_entry->joinleftcols = NIL;
            last_join_entry->joinrightcols = NIL;
            // TODO: This should probably be changed when handling traceprov attrs.
            foreach(target_var_cursor, first_rte->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(target_var_cursor));
                last_join_entry->joinaliasvars = lappend(last_join_entry->joinaliasvars, makeVarFromTargetEntry(1, te));
                col_names = lappend(col_names, makeString(te->resname));
                last_join_entry->joinleftcols = lappend_int(last_join_entry->joinleftcols, foreach_current_index(target_var_cursor) + 1);
            }

            target_var_cursor = NULL;
            foreach(target_var_cursor, next_rte->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(target_var_cursor));
                last_join_entry->joinaliasvars = lappend(last_join_entry->joinaliasvars, makeVarFromTargetEntry(1, te));
                col_names = lappend(col_names, makeString(te->resname));
                last_join_entry->joinrightcols = lappend_int(last_join_entry->joinrightcols, foreach_current_index(target_var_cursor) + 1);
            }
            last_join_entry->eref = makeAlias(pstrdup("unnamed join"), col_names);
            last_join_entry->rtekind = RTE_JOIN;
            join_expr = makeNode(JoinExpr);
            RangeTblRef *l_rtr = makeNode(RangeTblRef);
            RangeTblRef *r_rtr = makeNode(RangeTblRef);
            l_rtr->rtindex = 1;
            r_rtr->rtindex = 2;
            join_expr->larg = (Node*)l_rtr;
            join_expr->rarg = (Node*)r_rtr;
            join_expr->isNatural = false;
            join_expr->quals = createEqualityCondition(
                get_matchable_attrs(first_rte->subquery->targetList, ignore_list, 0), 
                get_matchable_attrs(next_rte->subquery->targetList, ignore_list, 1), 
                1, 
                2, 
                false
            );
        }else{
            RangeTblEntry *clonedRTE = copyObject(last_join_entry);
            const int originalLength = list_length(clonedRTE->joinaliasvars);
            col_names = clonedRTE->eref->colnames;
            clonedRTE->joinrightcols = NIL;
            clonedRTE->joinleftcols = NIL;
            foreach(target_var_cursor, next_rte->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(target_var_cursor));
                clonedRTE->joinaliasvars = lappend(clonedRTE->joinaliasvars, makeVarFromTargetEntry(1, te));
                col_names = lappend(col_names, makeString(te->resname));
                clonedRTE->joinrightcols = lappend_int(clonedRTE->joinrightcols, foreach_current_index(target_var_cursor) + 1);
            }
            for (int i = 0; i < originalLength; i++){
                clonedRTE->joinleftcols = lappend_int(clonedRTE->joinleftcols, i+1);
            }
            last_join_entry = clonedRTE;
            JoinExpr *next_join_expr = makeNode(JoinExpr);
            next_join_expr->larg = (Node*)join_expr;
            RangeTblRef *r_rtr = makeNode(RangeTblRef);
            r_rtr->rtindex = i;
            next_join_expr->rarg = (Node*)r_rtr;
            next_join_expr->isNatural = false;
            next_join_expr->quals = createEqualityCondition(
                get_matchable_attrs(first_rte->subquery->targetList, ignore_list, 0), 
                get_matchable_attrs(next_rte->subquery->targetList, ignore_list, i-1),
                1, 
                i, 
                false
            );
            join_expr = next_join_expr;
        }
        addedJoins = lappend(addedJoins, last_join_entry);
        join_expr->rtindex = list_length(base->rtable) + list_length(addedJoins);
    }
    FromExpr *fromExpr = makeFromExpr(list_make1(join_expr), NULL);
    base->rtable = list_concat_copy(base->rtable, addedJoins);
    base->jointree = fromExpr;
    return extra_targets_per_rte;
}
