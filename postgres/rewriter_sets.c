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

static List *getMatchableAttrs(List *rawList, List *ignoreList, int index){
    if (ignoreList == NIL) return rawList;
    if (index >= list_length(ignoreList)){
        elog(ERROR, "given index is out of bounds!");
    }
    List *matchables = NIL;
    List *ignoreForRTE =  (List*)list_nth(ignoreList, index);
    ListCell *outerCursor;
    foreach(outerCursor, rawList){
        TargetEntry *te = (TargetEntry *)lfirst(outerCursor);
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

List *adjustUnionSetOps(
    SetOperationStmt *root, 
    List *extraTargets,
    List *queryRteList,
    TraceProvParseContext *context
){
    if (root->op != SETOP_UNION){
        elog(ERROR, "Expected op to be of union!");
    }
    traceProvAssertEqualLength(list_make2(extraTargets, queryRteList));

    // Get the max number of extra targets that were added.
    unsigned int maxTargetListLength = 0;
    ListCell *rteCursor = NULL;
    foreach(rteCursor, queryRteList){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCursor);
        traceProvAssertIsSubquery(rte);
        Query *subquery = rte->subquery;
        traceProvAssertNoResJunk(subquery->targetList);
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
            List *nullList = traceProvGetNullList(padding, INT8OID, -1, InvalidOid);
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
            subquery->targetList = traceProvAppendAtResJunk(subquery->targetList, newTe);
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
                    NIL
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
                    tpTarget->sublinks
                );

            extraTargetsTyped = lappend(
                extraTargetsTyped,
                typedTpTarget
            );
            traceprovEntries = lappend(traceprovEntries, tpResolveEntry(
                tpTarget,
                &childGraphs,
                NULL
            ));
        }
        newExtraTargets = lappend(newExtraTargets, extraTargetsTyped);
        tp_add_set_graph_item(context, setNumber, makeTraceProvDependency(TP_LOG, 0, childGraphs, traceprovEntries));
    }

    List *setOpFlattened = traceProvFindUsedRefs((Node*)root, traceProvInclusiveNavigator);
    ListCell *opCursor;
    const unsigned int expectedLength = maxTargetListLength + 1;
    foreach(opCursor, setOpFlattened){
        Node *node = (Node*)lfirst(opCursor);
        if (IsA(node, SetOperationStmt)){
            SetOperationStmt *setOp = (SetOperationStmt *)node;
            int currentLength = traceProvAssertEqualLength(
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
                    traceProvDupOid(INT8OID, padding)
                );
                setOp->colCollations = list_concat(
                    setOp->colCollations,
                    traceProvDupOid(InvalidOid, padding)
                );
                setOp->colTypmods = list_concat(
                    setOp->colTypmods,
                    traceProvDupInt(-1, padding)
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
Query *adjustExceptSetOps(
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
    traceProvAssertEqualLength(list_make2(extraTargets, queryRteList));
    List *leftMostElement = traceProvFindUsedRefs((Node*)root, leftNavigator);
    // There should always be just one left-most element. assert thatn.
    traceProvAssertEqualLength(list_make2(leftMostElement, list_make1(NULL)));
    const RangeTblRef *leftMostRef = (RangeTblRef *)lfirst(list_head(leftMostElement));
    List *allSetElements = traceProvFindUsedRefs((Node*)root, traceProvInclusiveNavigator);
    
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
        Query *modified = traceprov_make_nested_query(rte->subquery, context);
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
        Query *modified = traceprov_make_nested_query(rte->subquery, context);
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
            traceProvAssertIsSubquery(rte);
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
                rte->subquery->targetList = traceProvAppendAtResJunk(rte->subquery->targetList, targetEntry);
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
                traceProvDupOid(InvalidOid, padding)
            );
            internalSetOp->colTypmods = list_concat(
                internalSetOp->colTypmods,
                traceProvDupInt(-1, padding)
            );
            internalSetOp->colTypes = list_concat(
                internalSetOp->colTypes,
                traceProvDupOid(tpPointerTypId, padding)
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
    List *childTargets = traceProvPropagateChildTargets(extraTargetsForRTE, 1);
    Query *parent = traceprov_make_nested_query(inputQuery, context);
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

Query *handleIntersect(Query *base, List *ignoreList){
    // Here, need to replace the intersect with a join.
    // For each table pair present, need to construct the join, and the join tree.
    // Basically, all the intersects within this block are consumed into a join.
    // It doesn't matter what join order is used (optimizer might reorder it anyways)
    // Simplest order is the one in RTElist, so it is directly used.
    // However, need to ignore the traceprov attributes in the join condition.
    // So, the ignoreList, if given, is consulted. If the table index AND column idx is same,
    // column is ignored from predicates.
    const List *usedReferences = traceProvFindUsedRefs(base->setOperations, traceProvLeafNavigator);
    if (list_length(usedReferences) != list_length(base->rtable)){
        elog(ERROR, "Expected the used references to be of the same size as setOperations");
    }
    if (list_length(usedReferences) < 2){
        elog(ERROR, "Expected at least 2 elements for the join!");
    }
    RangeTblEntry *firstRTE = (RangeTblEntry*) lfirst(list_head(base->rtable));
    if (firstRTE->rtekind != RTE_SUBQUERY) elog(ERROR, "Expected to be a subquery!");
    firstRTE->inFromCl = true;
    List *addedJoins = NIL;
    ListCell *rteCursor;
    RangeTblEntry *lastJoinEntry = NULL;
    JoinExpr *joinExpr = NULL;

    // Start from the next table.
    int i = 1;
    for_each_from(rteCursor, base->rtable, 1){
        i++;
        RangeTblEntry *nextRTE = (RangeTblEntry *)lfirst(rteCursor);
        if (nextRTE->rtekind != RTE_SUBQUERY) elog(ERROR, "Expected to be a subquery!");
        nextRTE->inFromCl = true;
        List *colNames = NIL;
        ListCell *targetVarCursor;
        if (lastJoinEntry == NULL){
            lastJoinEntry = makeNode(RangeTblEntry);
            lastJoinEntry->joinleftcols = NIL;
            lastJoinEntry->joinrightcols = NIL;
            // TODO: This should probably be changed when handling traceprov attrs.
            foreach(targetVarCursor, firstRTE->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(targetVarCursor));
                lastJoinEntry->joinaliasvars = lappend(lastJoinEntry->joinaliasvars, makeVarFromTargetEntry(1, te));
                colNames = lappend(colNames, makeString(te->resname));
                lastJoinEntry->joinleftcols = lappend_int(lastJoinEntry->joinleftcols, foreach_current_index(targetVarCursor) + 1);
            }

            targetVarCursor = NULL;
            foreach(targetVarCursor, nextRTE->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(targetVarCursor));
                lastJoinEntry->joinaliasvars = lappend(lastJoinEntry->joinaliasvars, makeVarFromTargetEntry(1, te));
                colNames = lappend(colNames, makeString(te->resname));
                lastJoinEntry->joinrightcols = lappend_int(lastJoinEntry->joinrightcols, foreach_current_index(targetVarCursor) + 1);
            }
            lastJoinEntry->eref = makeAlias(pstrdup("unnamed join"), colNames);
            lastJoinEntry->rtekind = RTE_JOIN;
            joinExpr = makeNode(JoinExpr);
            RangeTblRef *l_rtr = makeNode(RangeTblRef);
            RangeTblRef *r_rtr = makeNode(RangeTblRef);
            l_rtr->rtindex = 1;
            r_rtr->rtindex = 2;
            joinExpr->larg = (Node*)l_rtr;
            joinExpr->rarg = (Node*)r_rtr;
            joinExpr->isNatural = false;
            joinExpr->quals = createEqualityCondition(
                getMatchableAttrs(firstRTE->subquery->targetList, ignoreList, 0), 
                getMatchableAttrs(nextRTE->subquery->targetList, ignoreList, 1), 
                1, 
                2, 
                false
            );
        }else{
            RangeTblEntry *clonedRTE = copyObject(lastJoinEntry);
            const int originalLength = list_length(clonedRTE->joinaliasvars);
            colNames = clonedRTE->eref->colnames;
            clonedRTE->joinrightcols = NIL;
            clonedRTE->joinleftcols = NIL;
            foreach(targetVarCursor, nextRTE->subquery->targetList){
                TargetEntry *te = (TargetEntry *)(lfirst(targetVarCursor));
                clonedRTE->joinaliasvars = lappend(clonedRTE->joinaliasvars, makeVarFromTargetEntry(1, te));
                colNames = lappend(colNames, makeString(te->resname));
                clonedRTE->joinrightcols = lappend_int(clonedRTE->joinrightcols, foreach_current_index(targetVarCursor) + 1);
            }
            for (int i = 0; i < originalLength; i++){
                clonedRTE->joinleftcols = lappend_int(clonedRTE->joinleftcols, i+1);
            }
            lastJoinEntry = clonedRTE;
            JoinExpr *nextJoinExpr = makeNode(JoinExpr);
            nextJoinExpr->larg = (Node*)joinExpr;
            RangeTblRef *r_rtr = makeNode(RangeTblRef);
            r_rtr->rtindex = i;
            nextJoinExpr->rarg = (Node*)r_rtr;
            nextJoinExpr->isNatural = false;
            nextJoinExpr->quals = createEqualityCondition(
                getMatchableAttrs(firstRTE->subquery->targetList, ignoreList, 0), 
                getMatchableAttrs(nextRTE->subquery->targetList, ignoreList, i-1),
                1, 
                i, 
                false
            );
            joinExpr = nextJoinExpr;
        }
        addedJoins = lappend(addedJoins, lastJoinEntry);
        joinExpr->rtindex = list_length(base->rtable) + list_length(addedJoins);
    }
    FromExpr *fromExpr = makeFromExpr(list_make1(joinExpr), NULL);
    base->rtable = list_concat_copy(base->rtable, addedJoins);
    base->jointree = fromExpr;
    return base;
}