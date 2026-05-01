#include "postgres.h"
#include "rewriter_utils.h"
#include "parser/parse_node.h"
#include "nodes/makefuncs.h"
#include "traceprov_parse_context.h"
#include "catalog/pg_operator.h"
#include "parser/parse_oper.h"
#include "utils/syscache.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_func.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_authid_d.h"
#include "catalog/pg_language_d.h"
#include "utils/fmgroids.h"
#include "utils/builtins.h"
#include "pgstat.h"
#include "commands/defrem.h"
#include "catalog/pg_namespace_d.h"
#include "miscadmin.h"
#include "access/xact.h"
#include "rewrite/rewriteManip.h"
#include "nodes/pathnodes.h"
#include "parser/parse_relation.h"

TraceProvUsedRefNavigator traceProvInclusiveNavigator = {
    .includeInternals = true,
    .goLeft = true,
    .goRight = true
};

TraceProvUsedRefNavigator traceProvLeafNavigator = {
    .includeInternals = false,
    .goLeft = true,
    .goRight = true
};

void traceprov_assert_no_resjunk(const List *targetList){
    ListCell *targetlist_cursor;
    foreach(targetlist_cursor, targetList){
        const TargetEntry *entry = (TargetEntry *)lfirst(targetlist_cursor);
        if (entry->resjunk){
            elog(ERROR, "Expected no resjunk columns!");
        }  
    }
}

void traceprov_assert_is_subquery(const RangeTblEntry *rte){
    if (rte->rtekind != RTE_SUBQUERY){
        elog(ERROR, "Expected rte to be of subquery, got: %d", rte->rtekind);
    }
    if (rte->subquery == NULL){
        elog(ERROR, "Expected subquery to be non-null!");
    }
}

// Helper that also sets the resno of target entry appropriately.
List *_appendAndAdjustResno(List *in_list, TargetEntry *toAdd){
    List *new_list = lappend(in_list, toAdd);
    toAdd->resno = list_length(new_list);
    return new_list;
}

List *traceprov_append_at_resjunk(List *old, TargetEntry *newTe){
    if (list_length(old) == 0 || newTe->resjunk){
        // Append it to the very end.
        return lappend(old, newTe);
    }

    // Need to find the first resjunk, and then append it there.
    List *new_list = NIL;
    ListCell *cursor;
    bool has_seen_res_junk = false;
    foreach(cursor, old){
        TargetEntry *target_entry = (TargetEntry *)lfirst(cursor);
        if (target_entry->resjunk && !has_seen_res_junk){
            new_list = _appendAndAdjustResno(new_list, newTe);
            has_seen_res_junk = true;
        }
        new_list = _appendAndAdjustResno(new_list, target_entry);
    }
    if (!has_seen_res_junk){
        new_list = _appendAndAdjustResno(new_list, newTe);
    }
    Assert(list_length(new_list) == (list_length(old) + 1));
    return new_list;
}

List *traceprov_get_null_list(unsigned count, Oid consttype, int32 consttypmod, Oid constcollid){
    // This might be too strict, but should never get raised. We can still have defined behavior (return NIL)
    if (count == 0) elog(ERROR, "expected to get atleast 1!");
    List *nullList = NIL;
    for (unsigned int i = 0; i < count; i++){
        nullList = lappend(nullList, makeNullConst(consttype, consttypmod, constcollid));
    }
    return nullList;
}

// Checks that the length of the input lists are same.
// The input is a list of lists (to avoid functions for different number of args)
int traceprov_assert_equal_length(List *in_lists){
    if (list_length(in_lists) < 2){
        elog(ERROR, "expected at least 2 lists to be compared.");
    }

    const int firstLength = list_length((List *)(lfirst(list_head(in_lists))));
    ListCell *cursor;
    for_each_from(cursor, in_lists, 1){
        const List *in_list = (List *)lfirst(cursor);
        if (list_length(in_list) != firstLength){
            elog(ERROR, "got list length to be different!");
        }
    }
    return firstLength;
}

// Takes in the head and gets all the leaf refs.
// This is useful when some action needs to be performed on the flattened structure.
// If includeInternals is true, also includes the internal node (rather than just the leaf refs.)
List *traceprov_find_used_refs(Node *head, TraceProvUsedRefNavigator navigator){
    if (head == NULL){
        // Nothing to do.
        return NIL;
    }
    if (!IsA(head, RangeTblRef) && !IsA(head, SetOperationStmt)){
        elog(ERROR, "Expected either range table ref or set operation stmt. Got : %d", head->type);
    }
    // No need to do anything.
    if (IsA(head, RangeTblRef)) return list_make1(head);
    SetOperationStmt *setOp = (SetOperationStmt *)head;

    List *refs = NIL;
    if (navigator.goLeft){
        refs = list_concat_copy(refs, traceprov_find_used_refs(setOp->larg, navigator));
    }
    if (navigator.goRight){
        refs = list_concat_copy(refs, traceprov_find_used_refs(setOp->rarg, navigator));
    }

    if (navigator.includeInternals) refs = lappend(refs, head);
    return refs;
}

List *traceprov_dup_int(int element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_int(begin, element);
    }
    return begin;
}

List *traceprov_dup_oid(Oid element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_oid(begin, element);
    }
    return begin;
}
List *traceprov_flatten(List *in_list){

    List *new_list = NIL;
    
    ListCell *outer_cursor;
    foreach(outer_cursor, in_list){
        List *inner_list = lfirst(outer_cursor);
        new_list = list_concat(new_list, inner_list);
    }
    return new_list;
}

List* traceprov_append_targets(List *traceProvTargets, List *targetList){
    ListCell *target_entry_cursor;
    foreach(target_entry_cursor, traceProvTargets){
        // Add target entry to the parse->targetlist.
        TargetEntry *target = ((TraceProvTarget *)lfirst(target_entry_cursor))->targetEntry;
        // Here is an ugly case.
        // It is possible that the attributes we're grouping over don't appear as resjunk.
        // In that case, we'll need to adjust the references in the sort refs.
        targetList = traceprov_append_at_resjunk(targetList, target);
    }
    return targetList;
}

const TraceProvTarget *traceprov_find_matching_set_pointer(List *traceProvTargets, int setNumber){
    ListCell *traceProvTargetCursor;
    foreach(traceProvTargetCursor, traceProvTargets){
        const TraceProvTarget *tpTarget = ((TraceProvTarget *)lfirst(traceProvTargetCursor));
        if (tpTarget->isSetPointer == true && tpTarget->setNumber == setNumber){
            return tpTarget;
        }
    }
    elog(ERROR, "No matching candidate found for %d", setNumber);
    return NULL;
}

Node *
createNotDistinctConditionForVars (Var *leftChild, Var *rightChild)
{
    Form_pg_operator operator;
    DistinctExpr *equal;
    Expr *notExpr;
    HeapTuple tup = NULL;
    Operator operTuple = NULL;
    Oid eqOpOid;

    get_sort_group_operators(leftChild->vartype, false, true, false, NULL, &eqOpOid, NULL, NULL);

    if (OidIsValid(eqOpOid))
    {
        tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(eqOpOid));
        if (HeapTupleIsValid(tup)){
            operTuple = (Operator)tup;
        }else{
            elog(ERROR, "Heap tuple not valid!");
        }
    }else{
        elog(ERROR, "Got invalid equipid!");
    }

    operator = (Form_pg_operator) GETSTRUCT(operTuple);

    equal = makeNode (DistinctExpr);
    equal->args = list_make2(leftChild, rightChild);
    equal->opfuncid = operator->oprcode;
    equal->opno = eqOpOid;
    equal->opresulttype = operator->oprresult;
    equal->opretset = false;

    ReleaseSysCache (tup);

    notExpr = makeBoolExpr(NOT_EXPR, list_make1(equal), -1);

    return (Node *) notExpr;
}

/*
 * Creates a logical AND expression for a list of expressions.
 */

Node *
createAndFromList (List *exprs)
{
    ListCell *lc;
    Node *node;
    Node *result;

    if (list_length(exprs) == 0)
        return NULL;
    if (list_length(exprs) == 1)
        return (Node *) linitial(exprs);

    result = (Node *) linitial(exprs);

    for_each_from(lc, exprs, 1){
        node = (Node *) lfirst(lc);
        result = (Node *) makeBoolExpr(AND_EXPR, list_make2(node, result), -1);
    }

    return result;
}

/*
 * Creates an equality condition expression for two lists of attributes.
 * E.g. A = (a,b,c) and B = (d,e,f) then the following condition would be created:
 * a = d AND b = e AND c = f. If neq is true the whole condition is negated.
 */

Node *
createEqualityCondition (List* leftAttrs, List* rightAttrs, Index leftIndex, Index rightIndex, bool neq)
{
    ListCell *leftLc;
    ListCell *rightLc;
    TargetEntry *curLeft;
    TargetEntry *curRight;
    OpExpr *equal;
    List *equalConds;
    Var *leftOp;
    Var *rightOp;
    Node *curRoot;

    equalConds = NIL;

    Assert (list_length(leftAttrs) == list_length(rightAttrs));

    /* create List of OpExpr nodes for equality conditions */
    forboth (leftLc, leftAttrs, rightLc, rightAttrs)
    {
        curLeft = (TargetEntry *) lfirst(leftLc);
        curRight = (TargetEntry *) lfirst(rightLc);

        /* create Var for left operand of equality expr */
        leftOp = makeVar (leftIndex + 1,
                curLeft->resno,
                exprType ((Node *) curLeft->expr),
                exprTypmod ((Node *) curLeft->expr),
                exprCollation((Node *) curLeft->expr),
                0);

        /* create Var for right operand of equality expr */
        rightOp = makeVar (rightIndex + 1,
                curRight->resno,
                exprType ((Node *) curRight->expr),
                exprTypmod ((Node *) curRight->expr),
                exprCollation ((Node *) curRight->expr),
                0);

        /* get equality operator for the var's type */
        equal = (OpExpr *) createNotDistinctConditionForVars (leftOp, rightOp);

        /* append current equality condition to equalConds List */
        equalConds = lappend (equalConds, equal);
    }

    curRoot = (Node *) createAndFromList(equalConds);

    /* negation required */
    if (neq)
        curRoot = (Node *) makeBoolExpr(NOT_EXPR, list_make1(curRoot), -1);

    return curRoot;
}

Query *traceprov_clone_query(const Query *base){
    Query *targetQuery = makeNode(Query);
    targetQuery->commandType = base->commandType;
    targetQuery->querySource = base->querySource;
    targetQuery->queryId = base->queryId;
    targetQuery->canSetTag = base->canSetTag;
    targetQuery->utilityStmt = base->utilityStmt;
    targetQuery->resultRelation = base->resultRelation;
    // We're no longer an aggregation!
    targetQuery->hasAggs = false;
    targetQuery->hasWindowFuncs = false;
    // TODO: Look at target srfs, when will we get evaulated in that case?
    targetQuery->hasTargetSRFs = false;
    targetQuery->hasSubLinks = false;
    targetQuery->hasDistinctOn = false;
    targetQuery->hasRecursive = false;
    targetQuery->hasModifyingCTE = base->hasModifyingCTE;
    targetQuery->hasForUpdate = base->hasForUpdate;
    targetQuery->hasRowSecurity = base->hasRowSecurity;
    // TODO: Look at this more.
    targetQuery->isReturn = base->isReturn;
    // TODO: Think more about CTEs.
    // Technically, we can nest the entire block...
    targetQuery->cteList = base->cteList;
    // TODO: Set this.
    targetQuery->rtable = NIL;
    // TODO: Set this.
    targetQuery->jointree = NULL;
    // TODO: Set this.
    targetQuery->targetList = NIL; 
    // TODO: Investigate.
    targetQuery->override = base->override;
    targetQuery->onConflict = base->onConflict;
    // TODO: Investigate.
    targetQuery->returningList = base->returningList;
    // TODO: Investigate.
    targetQuery->groupClause = NIL;
    // TODO: Investigate.
    targetQuery->groupDistinct = false;
    // TODO: Investigate.
    targetQuery->groupingSets = NIL;
    targetQuery->havingQual = NULL;
    targetQuery->windowClause = NIL;
    targetQuery->distinctClause = NIL;
    targetQuery->sortClause = NIL;
    targetQuery->limitOffset = NULL;
    targetQuery->limitCount = NULL;
    targetQuery->limitOption = 0;
    targetQuery->rowMarks = NIL;
    targetQuery->setOperations = NULL;
    targetQuery->constraintDeps = NIL;
    targetQuery->withCheckOptions = NIL;
    targetQuery->stmt_location = base->stmt_location;
    targetQuery->stmt_len = base->stmt_len;
    return targetQuery;
}

RangeTblEntry *range_table_entry_from_subquery(Query *sub_query, TraceProvParseContext *context, bool copy_resjunk){
    RangeTblEntry *tblEntry = makeNode(RangeTblEntry);
    char *aliasName = tp_parse_get_unique_alias(context);
    tblEntry->alias = makeAlias(aliasName, NIL);
    List *colNames = NIL;
    ListCell *target_entry_cursor = NULL;
    foreach(target_entry_cursor, sub_query->targetList){
        TargetEntry *te = (TargetEntry *)lfirst(target_entry_cursor);
        if (te->resjunk && !copy_resjunk) continue;
        colNames = lappend(colNames, makeString(te->resname == NULL ? (tp_parse_get_unique_alias(context)) : pstrdup(te->resname)));
    }
    tblEntry->eref = makeAlias(pstrdup(aliasName), colNames);
    // Doesn't seem like this will have any side-effects (at this stage at least)
    tblEntry->inFromCl = false;
    tblEntry->rtekind = RTE_SUBQUERY;
    tblEntry->subquery = sub_query;
    return tblEntry;
}

FromExpr *traceprov_make_from_expr(RangeTblEntry *rte){
    rte->inFromCl = true;
    RangeTblRef *rtr = makeNode(RangeTblRef);
    rtr->rtindex = 1;
    return makeFromExpr(list_make1(rtr), NULL);
}

Query *traceprov_make_nested_query(Query *base, TraceProvParseContext *context, bool copy_resjunk, bool copy_sorted_order){
    Query *modified = traceprov_clone_query(base);
    ListCell *target_entry_cursor;
    foreach(target_entry_cursor, base->targetList){
        TargetEntry *base_target_entry = (TargetEntry *)lfirst(target_entry_cursor);
        if (base_target_entry->resjunk && !copy_resjunk) continue;
        TargetEntry *new_te = makeTargetEntry(
            (Expr*)makeVarFromTargetEntry(1, base_target_entry),
            base_target_entry->resno,
            (base_target_entry->resname == NULL ? NULL : pstrdup(base_target_entry->resname)),
            false
        );
        if (copy_sorted_order) new_te->ressortgroupref = base_target_entry->ressortgroupref;
        modified->targetList = lappend(modified->targetList, new_te);
    }
    RangeTblEntry *rte = range_table_entry_from_subquery(base, context, copy_resjunk);
    modified->jointree = traceprov_make_from_expr(rte);
    modified->rtable = list_make1(rte);
    // Increment all the nested vars in the original query.
    IncrementVarSublevelsUp((Node*)base, 1, 1);
    return modified;
}

void zip_target_sortgroupclause(List *group_clauses, List *target_list){
    int i = 0;
    ListCell *group_clause_cursor;
    ListCell *target_list_cursor;
    forboth(group_clause_cursor, group_clauses, target_list_cursor, target_list){
        i++;
        SortGroupClause *group_clause = (SortGroupClause *)lfirst(group_clause_cursor);
        TargetEntry *target_entry = (TargetEntry *)lfirst(target_list_cursor);
        group_clause->tleSortGroupRef = i;
        target_entry->ressortgroupref = i; 
    }
}

List *traceprov_aggregate_on_set(
    const SetOperationStmt *stmt, 
    Query *query,
    TraceProvParseContext *context,
    bool aggregatedInParent,
    List *newTargetList,
    List *addedTargets
){
    elog(INFO, "Trying to make a distinct in setop!");
    // Need to add a distinct operation.
    // Here, actually, group-by is used (rather than distinct), because that's what traceprov will use.
    if (list_length(stmt->groupClauses) == 0){
        elog(ERROR, "Expected length of group clauses to defined, when not all!");
    }
    if (list_length(query->groupClause) != 0){
        elog(INFO, "Expected no aggregates already, in the modified group clause");
    }
    query->groupClause = list_copy_deep(stmt->groupClauses);
    zip_target_sortgroupclause(query->groupClause, query->targetList);
    query->targetList = newTargetList;
    List *aggregated = NIL;
    traceprov_aggregate_rewrite(
        addedTargets,
        &aggregated,
        context,
        aggregatedInParent,
        NULL,
        NULL,
        false,
        // This could never be a purely sink agg, since that'll imply that there are
        // no columns in the select clause...
        false
    );
    query->hasAggs = true;
    query->targetList = traceprov_append_targets(aggregated, query->targetList);
    return aggregated;
}

bool traceprov_find_int_list(List *in_list, int to_find){
    ListCell *list_cursor;
    foreach(list_cursor, in_list){
        if(lfirst_int(list_cursor) == to_find) return true;
    }
    return false;
}

bool traceprov_find_oid_list(List *in_list, Oid to_find){
    ListCell *list_cursor;
    foreach(list_cursor, in_list){
        if(lfirst_oid(list_cursor) == to_find) return true;
    }
    return false;
}

Node *traceprov_get_function_call_node(
    const char *func_name, 
    List *argVars,
    WindowDef *over
){
    ParseState *dummyParseState = make_parsestate(NULL);
    List *func_name_list = list_make1(makeString(pstrdup(func_name)));
    FuncCall *fc = makeFuncCall(func_name_list, argVars, COERCE_EXPLICIT_CALL, -1);
    fc->over = over;
    if (over){
        // We know we're valid. Tell Postgres that.
        dummyParseState->p_expr_kind = EXPR_KIND_OTHER;
    }
    Node *fcNode =  ParseFuncOrColumn(
        dummyParseState,
        func_name_list,
        argVars,
        NULL,
        fc,
        false,
        fc->location
    );
    free_parsestate(dummyParseState); 
    return fcNode;
}


List *traceprov_prepare_arg_vars(TraceProvParseContext *context, TraceProvLayerNumber *out_layer_number){
    TraceProvLayerNumber layer_number = tp_parse_get_layer_number(context);
    // Need to also add the layer number (the first argument)
    Node * layerNumberConst = (Node *) makeConst(
        INT4OID, 
        -1, 
        InvalidOid,
        sizeof(int32),
        Int32GetDatum(layer_number), 
        false,
        true
    );
    // The layer number is the first argument.
    List *argVars = list_make1(layerNumberConst);
    *out_layer_number = layer_number;
    return argVars;
}

TraceProvLayerNumber traceprov_aggregate_rewrite(
    const List *targetEntriesToLog,
    List **pCreatedTargets,
    TraceProvParseContext *tpContext,
    bool parentHasAggs,
    WindowDef *over,
    // Useful because in some places (window rewrites)
    // we need to have a reference to the function node.
    Node **fc_node,
    bool is_for_window,
    bool is_sink_agg
){
    // Need to add the exprs from the targets.
    ListCell *target_entry_cursor;
    TraceProvLayerNumber layer_number;
    List *argVars = traceprov_prepare_arg_vars(tpContext, &layer_number);
    List *entries = NIL;
    List *childGraphs = NIL;
    List *remainingArgVars = NIL;
    foreach(target_entry_cursor, targetEntriesToLog){
        TraceProvEntry *tpEntry = traceprov_resolve_entry(((TraceProvTarget *)lfirst(target_entry_cursor)), &childGraphs, &remainingArgVars);
        entries = lappend(entries, tpEntry);
    }

    const uint64 null_bit_map = get_null_entry_map(entries);
    argVars = lappend(argVars, makeInt8Const(null_bit_map));
    argVars = list_concat(argVars, remainingArgVars);
    // Need to make the func exprn.
    // Postgres' parser has all the logic already to determine functions,
    // and even adding type castes when types can be implicitly converted (like int->bigint)
    // So, the logic is just reused by making a fake parse state and then just calling Postgres'
    // parser on that.
    // TODO: Think about mixed cases (say, first three are pointers, next two are ints.)
    // In this case, we might be able to reuse the pointers.
    // TODO: Re-use pointers, rather than relogging them.
    // Always use the offset version no matter what.
    Node *funcCallNode = traceprov_get_function_call_node(
        TRACEPROV_AGG_OFFSETS_FUNC_NAME,
        argVars,
        over
    );
    if ((over == NULL && !IsA(funcCallNode, Aggref)) || (over != NULL && !IsA(funcCallNode, WindowFunc))){
        elog(ERROR, "Expected the function call node to be an aggref!");
    }
    if (fc_node) *fc_node = funcCallNode;
    *pCreatedTargets = list_make1(
        makeTraceProvTarget(
            true,
            makeTargetEntry(
                (Expr*) funcCallNode,
                0,
                pstrdup("mapped_agg"),
                false
            ),
            make_traceprov_dependency(
                is_sink_agg ? TP_PURE_AGGREGATE : TP_AGGREGATE,
                layer_number,
                childGraphs,
                entries
            ),
            0,
            false,
            NIL,
            NULL,
            is_for_window,
            false
        )
    );
    GET_ROOT_CONTEXT(tpContext)->properties->traceprov_funcs = lappend_oid(GET_ROOT_CONTEXT(tpContext)->properties->traceprov_funcs, ((Aggref *) funcCallNode)->aggfnoid);
    return layer_number;
}

Const *makeInt8Const(int64 value){
    return makeConst(
        INT8OID,
        -1,
        InvalidOid,
        sizeof(int64),
        Int64GetDatum(value),
        false,
        true
    );
}

// Makes new traceprov targets to be used in the parent.
List *traceprov_propagate_child_targets(List *childTargets, Index rteIndex){
    List *targetsToAdd = NIL;
    ListCell *childTargetCell;
    foreach(childTargetCell, childTargets){
        TraceProvTarget *tpTarget = (TraceProvTarget *)lfirst(childTargetCell);
        TargetEntry *childTarget = tpTarget->targetEntry;
        const Var *newVar = makeVarFromTargetEntry(rteIndex, childTarget);
        TargetEntry *newTarget = makeTargetEntry(
            (Expr *)newVar, childTarget->resno, childTarget->resname, false
        );
        newTarget->resorigcol = childTarget->resorigcol;
        newTarget->resorigtbl = childTarget->resorigtbl;
        // Propogate the graph (if there is one)
        // That can happen, for example, if for example the aggregation is nested inside a subquery.
        targetsToAdd = lappend(
            targetsToAdd, 
            makeTraceProvTarget(
                tpTarget->isPointer,
                newTarget,
                tpTarget->graph,
                tpTarget->setNumber,
                tpTarget->isSetPointer,
                tpTarget->sublinks,
                tpTarget->window_entry,
                tpTarget->is_pointer_for_window,
                tpTarget->is_nullable
            )
        );
    }
    return targetsToAdd;
}

Datum
dummy_pg_get_function_sqlbody(PG_FUNCTION_ARGS)
{
    Oid            funcid = PG_GETARG_OID(0);
    StringInfoData buf;
    HeapTuple    proctup;
    bool        isnull;

    initStringInfo(&buf);

    /* Look up the function */
    proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcid));
    if (!HeapTupleIsValid(proctup))
        PG_RETURN_NULL();

    (void) SysCacheGetAttr(PROCOID, proctup, Anum_pg_proc_prosqlbody, &isnull);
    if (isnull)
    {
        ReleaseSysCache(proctup);
        PG_RETURN_NULL();
    }

    Datum result = pg_get_function_sqlbody(fcinfo);

    ReleaseSysCache(proctup);

    return result;
}

// For some reason, it doesn't work well on > 14.
// Temporarily, just return invalid.
// Everything else just picks this repr up so they don't need to be changed
#if (PG_MAJORVERSION_NUM > 14)
// char *tracprov_parse_back_query(Query *query){
//     return pstrdup("--invalid-parsed-back--");
// }
// #else
// Get str representation of the query.
// This, first, makes a function out of the query.
// Then, looks at the SQL body of the function.
// It just, then, calls the existing postgres utility to parse back.
char *tracprov_parse_back_query(Query *query){
    Query *cloned = copyObject(query);
    ObjectAddress created = ProcedureCreate(
        pstrdup("traceprovquery"),
        PG_PUBLIC_NAMESPACE,
        true,
        false,
        INT4OID,
        GetUserId(),
        INTERNALlanguageId,
        InvalidOid,
        "traceprov_dummy",
        NULL,
        (Node*)cloned,
        PROKIND_FUNCTION,
        false,
        false,
        false,
        PROVOLATILE_IMMUTABLE,
        PROPARALLEL_SAFE,
        buildoidvector(NULL, 0),
        PointerGetDatum(NULL), /* allParameterTypes */
        PointerGetDatum(NULL), /* parameterModes */
        PointerGetDatum(NULL), /* parameterNames */
        NIL,    /* parameterDefaults */
        PointerGetDatum(NULL), /* trftypes */
        PointerGetDatum(NULL), /* proconfig */
        InvalidOid,    /* prosupport */
        1.0,    /* procost */
        0.0    /* prorows */
    );
    CommandCounterIncrement();
    text *response = (DatumGetTextP(DirectFunctionCall1(dummy_pg_get_function_sqlbody, ObjectIdGetDatum(created.objectId))));
    char *str = text_to_cstring(response);
    elog(INFO, "parsed back: %s", str);
    return str;
}
#endif

typedef struct
{
    List *vars;
    int sublevels_up;
} pull_vars_context;

static bool
pull_vars_of_level_ignore_sublinks_walker(Node *, pull_vars_context *);

// Like postgres' default pull_vars_of_level, but ignores sublinks.
List *
pull_vars_of_level_ignore_sublinks(Node *node, int levelsup)
{
    pull_vars_context context;

    context.vars = NIL;
    context.sublevels_up = levelsup;

    /*
     * Must be prepared to start with a Query or a bare expression tree; if
     * it's a Query, we don't want to increment sublevels_up.
     */
    query_or_expression_tree_walker(node,
                                    pull_vars_of_level_ignore_sublinks_walker,
                                    (void *) &context,
                                    0);

    return context.vars;
}


static bool
pull_vars_of_level_ignore_sublinks_walker(Node *node, pull_vars_context *context)
{
    if (node == NULL)
        return false;

    // Don't do anything if node is a sublink.    
    if (IsA(node, SubLink))
        return false;

    if (IsA(node, Var))
    {
        Var           *var = (Var *) node;

        if (var->varlevelsup == context->sublevels_up)
            context->vars = lappend(context->vars, var);
        return false;
    }
    if (IsA(node, PlaceHolderVar))
    {
        PlaceHolderVar *phv = (PlaceHolderVar *) node;

        if (phv->phlevelsup == context->sublevels_up)
            context->vars = lappend(context->vars, phv);
        /* we don't want to look into the contained expression */
        return false;
    }
    if (IsA(node, Query))
    {
        /* Recurse into RTE subquery or not-yet-planned sublink subquery */
        bool        result;

        context->sublevels_up++;
        result = query_tree_walker((Query *) node, pull_vars_of_level_ignore_sublinks_walker,
                                   (void *) context, 0);
        context->sublevels_up--;
        return result;
    }
    return expression_tree_walker(node, pull_vars_of_level_ignore_sublinks_walker,
                                  (void *) context);
}

// Pushes down a query.
// Basically, clones the original query and flattens all the vars from the range tbl.
// Also maintains the offset at which an attr got shifted.
Query *traceprov_push_down_query(Query *query, List **shift_spec){
    // Make a fresh query, since we'll be doing custom things.
    Query *cloned = makeNode(Query);
    cloned->commandType = CMD_SELECT;
    cloned->rtable = copyObject(query->rtable);
    cloned->jointree = copyObject(query->jointree);
    cloned->canSetTag = true;
    cloned->hasWindowFuncs = false;
    cloned->targetList = NIL;
    cloned->rteperminfos = query->rteperminfos;
    query->rteperminfos = NIL;
    List *new_targets = NIL;
    List *shifted_refs = NIL;
    ListCell *cursor = NULL;
    foreach(cursor, cloned->rtable){
        RangeTblEntry *rte = lfirst_node(RangeTblEntry, cursor);
        // Don't clone join table entries..
        if (rte->rtekind == RTE_JOIN) continue;
        shifted_refs = lappend_int(shifted_refs, list_length(new_targets));
        List *col_names = NIL;
        List *col_vars = NIL;
        expandRTE(rte, foreach_current_index(cursor) + 1, 0, -1, true, &col_names, &col_vars);
        ListCell *col_var_cursor = NULL;
        foreach(col_var_cursor, col_vars){
            const int idx = foreach_current_index(col_var_cursor);
            TargetEntry *te = makeTargetEntry((Expr *)lfirst_node(Var, col_var_cursor), 0, pstrdup(strVal(list_nth_node(Node, col_names, idx))), false);
            new_targets = traceprov_append_at_resjunk(new_targets, te);
        }
    }
    cloned->targetList = new_targets;
    // Because this query will be nested.
    IncrementVarSublevelsUp((Node*)cloned, 1, 1);
    *shift_spec = shifted_refs;
    return cloned;
}

List* traceprov_assert_all_vars(List *target_list){
    ListCell *cursor;
    List *vars = NIL;
    foreach(cursor, target_list){
        TargetEntry *te = lfirst_node(TargetEntry, cursor);
        if (!IsA(te->expr, Var)) elog(ERROR, "Expected all vars!");
        vars = lappend(vars, te->expr);
    }
    return vars;
}

List *traceprov_reverse_list(const List *original_list){
    List *reversed_list = NIL;
    ListCell *cursor;
    foreach(cursor, original_list){
        reversed_list = list_insert_nth(reversed_list, 0, lfirst(cursor));
    }
    if (list_length(reversed_list) != list_length(original_list))
        elog(ERROR, "Reversing creating list of different lengths!");
    return reversed_list;
}


static void
markRelsAsNulledBy(List **nulling_rels, Node *n, int jindex);

List *get_nulling_set(FromExpr *from_expr){
    List *nulling_rels = NIL;
    ListCell *join_cursor;
    foreach(join_cursor, from_expr->fromlist){
        const Node *curr_node = lfirst_node(Node, join_cursor);
        if (IsA(curr_node, JoinExpr)){
            const JoinExpr *j = castNode(JoinExpr, curr_node);
            switch (j->jointype)
            {
                case JOIN_INNER:
                    break;
                case JOIN_LEFT:
                    markRelsAsNulledBy(&nulling_rels, j->rarg, j->rtindex);
                    break;
                case JOIN_FULL:
                    markRelsAsNulledBy(&nulling_rels, j->larg, j->rtindex);
                    markRelsAsNulledBy(&nulling_rels, j->rarg, j->rtindex);
                    break;
                case JOIN_RIGHT:
                    markRelsAsNulledBy(&nulling_rels, j->larg, j->rtindex);
                    break;
                default:
                    /* shouldn't see any other types here */
                    elog(ERROR, "unrecognized join type: %d",
                            (int) j->jointype);
                    break;
            }
        }
    }
    return nulling_rels;
}

/*
 * markRelsAsNulledBy -
 *      Mark the given jointree node and its children as nulled by join jindex
 *  NOTE: Taken from Postgres. It's a static func unfortunately...
 */
static void
markRelsAsNulledBy(List **nulling_rels, Node *n, int jindex)
{
    int            varno;
    ListCell   *lc;

    /* Note: we can't see FromExpr here */
    if (IsA(n, RangeTblRef))
    {
        varno = ((RangeTblRef *) n)->rtindex;
    }
    else if (IsA(n, JoinExpr))
    {
        JoinExpr   *j = (JoinExpr *) n;

        /* recurse to children */
        markRelsAsNulledBy(nulling_rels, j->larg, jindex);
        markRelsAsNulledBy(nulling_rels, j->rarg, jindex);
        varno = j->rtindex;
    }
    else
    {
        elog(ERROR, "unrecognized node type: %d", (int) nodeTag(n));
        varno = 0;                /* keep compiler quiet */
    }

    /*
     * Now add jindex to the p_nullingrels set for relation varno.  Since we
     * maintain the p_nullingrels list lazily, we might need to extend it to
     * make the varno'th entry exist.
     */
    List *nulling_rels_copy = *nulling_rels;
    while (list_length(nulling_rels_copy) < varno)
        nulling_rels_copy = lappend(nulling_rels_copy, NULL);
    lc = list_nth_cell(nulling_rels_copy, varno - 1);
    lfirst(lc) = bms_add_member((Bitmapset *) lfirst(lc), jindex);
    *nulling_rels = nulling_rels_copy;
}

uint64 get_null_entry_map(List *entries){
    ListCell *entry_cursor;
    uint64 null_bit_map = 0;
    foreach(entry_cursor, entries){
        TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
        if (unlikely(entry->is_nullable)){
            null_bit_map |= (foreach_current_index(entry_cursor) + 1);
        }
    }
    return null_bit_map;
}

uint64 get_null_targets_map(List *entries){
    ListCell *entry_cursor;
    uint64 null_bit_map = 0;
    foreach(entry_cursor, entries){
        TraceProvTarget *entry = (TraceProvTarget *)lfirst(entry_cursor);
        traceprov_target_set_nullable(entry);
        if (unlikely(entry->is_nullable)){
            null_bit_map |= (foreach_current_index(entry_cursor) + 1);
        }
    }
    return null_bit_map;
}

// Returns a list which is composed of the address to the ptr values.
// Postgres implementation does add the condition that the initial list
// is not directly altered (appended, deleted), throughout the life cycle of the returned list
// Otherwise, the addresses may not be valid anymore (due to shifting.)
List *traceprov_get_ref_list(List *initial_list){
    List *ref_list = NIL;
    ListCell *cursor;
    foreach(cursor, initial_list){
        ref_list = lappend(ref_list, &cursor->ptr_value);
    }
    return ref_list;
}

// Filters out traceprov targets used in-correlation (they don't really resemble correlation, so don't need to be considered for reference)
List *traceprov_filter_in_correlation(List *traceprov_targets){
    List *filtered = NIL;
    ListCell *cursor;
    foreach(cursor, traceprov_targets){
        TraceProvTarget *tp_target = (TraceProvTarget *)lfirst(cursor);
        if (tp_target->is_in_correlation) continue;
        filtered = lappend(filtered, tp_target);
    }
    return filtered;
}