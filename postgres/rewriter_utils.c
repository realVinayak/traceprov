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

void traceProvAssertNoResJunk(const List *targetList){
    ListCell *targetListCursor;
    foreach(targetListCursor, targetList){
        const TargetEntry *entry = (TargetEntry *)lfirst(targetListCursor);
        if (entry->resjunk){
            elog(ERROR, "Expected no resjunk columns!");
        }  
    }
}

void traceProvAssertIsSubquery(const RangeTblEntry *rte){
    if (rte->rtekind != RTE_SUBQUERY){
        elog(ERROR, "Expected rte to be of subquery, got: %d", rte->rtekind);
    }
    if (rte->subquery == NULL){
        elog(ERROR, "Expected subquery to be non-null!");
    }
}

// Helper that also sets the resno of target entry appropriately.
List *_appendAndAdjustResno(List *inList, TargetEntry *toAdd){
    List *newList = lappend(inList, toAdd);
    toAdd->resno = list_length(newList);
    return newList;
}

List *traceProvAppendAtResJunk(List *old, TargetEntry *newTe){
    if (list_length(old) == 0 || newTe->resjunk){
        // Append it to the very end.
        return lappend(old, newTe);
    }

    // Need to find the first resjunk, and then append it there.
    List *newList = NIL;
    ListCell *cursor;
    bool hasSeenResJunk = false;
    foreach(cursor, old){
        TargetEntry *targetEntry = (TargetEntry *)lfirst(cursor);
        if (targetEntry->resjunk && !hasSeenResJunk){
            newList = _appendAndAdjustResno(newList, newTe);
            hasSeenResJunk = true;
        }
        newList = _appendAndAdjustResno(newList, targetEntry);
    }
    if (!hasSeenResJunk){
        newList = _appendAndAdjustResno(newList, newTe);
    }
    Assert(list_length(newList) == (list_length(old) + 1));
    return newList;
}

List *traceProvGetNullList(unsigned count, Oid consttype, int32 consttypmod, Oid constcollid){
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
int traceProvAssertEqualLength(List *inLists){
    if (list_length(inLists) < 2){
        elog(ERROR, "expected at least 2 lists to be compared.");
    }

    const int firstLength = list_length((List *)(lfirst(list_head(inLists))));
    ListCell *cursor;
    for_each_from(cursor, inLists, 1){
        const List *inList = (List *)lfirst(cursor);
        if (list_length(inList) != firstLength){
            elog(ERROR, "got list length to be different!");
        }
    }
    return firstLength;
}

// Takes in the head and gets all the leaf refs.
// This is useful when some action needs to be performed on the flattened structure.
// If includeInternals is true, also includes the internal node (rather than just the leaf refs.)
List *traceProvFindUsedRefs(Node *head, TraceProvUsedRefNavigator navigator){
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
        refs = list_concat_copy(refs, traceProvFindUsedRefs(setOp->larg, navigator));
    }
    if (navigator.goRight){
        refs = list_concat_copy(refs, traceProvFindUsedRefs(setOp->rarg, navigator));
    }

    if (navigator.includeInternals) refs = lappend(refs, head);
    return refs;
}

List *traceProvDupInt(int element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_int(begin, element);
    }
    return begin;
}

List *traceProvDupOid(Oid element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_oid(begin, element);
    }
    return begin;
}
List *traceProvFlatten(List *inList){

    List *newList = NIL;
    
    ListCell *outerCursor;
    foreach(outerCursor, inList){
        List *innerList = lfirst(outerCursor);
        ListCell *innerCursor;
        foreach(innerCursor, innerList){
            newList = lappend(newList, lfirst(innerCursor));
        }
    }
    return newList;
}

List* traceProvAppendTargets(List *traceProvTargets, List *targetList){
    ListCell *targetEntryCursor;
    foreach(targetEntryCursor, traceProvTargets){
        // Add target entry to the parse->targetlist.
        TargetEntry *target = ((TraceProvTarget *)lfirst(targetEntryCursor))->targetEntry;
        // Here is an ugly case.
        // It is possible that the attributes we're grouping over don't appear as resjunk.
        // In that case, we'll need to adjust the references in the sort refs.
        targetList = traceProvAppendAtResJunk(targetList, target);
    }
    return targetList;
}

const TraceProvTarget *traceProvFindMatchingSetPointer(List *traceProvTargets, int setNumber){
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
	Operator operTuple;
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

Query *cloneQueryForTP(const Query *base){
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

RangeTblEntry *rangeTableEntryFromSubquery(Query *subQuery, TraceProvParseContext *context){
    RangeTblEntry *tblEntry = makeNode(RangeTblEntry);
    char *aliasName = tpParseGetUniqueAlias(context);
    tblEntry->alias = makeAlias(aliasName, NIL);
    List *colNames = NIL;
    ListCell *targetEntryCursor = NULL;
    foreach(targetEntryCursor, subQuery->targetList){
        TargetEntry *te = (TargetEntry *)lfirst(targetEntryCursor);
        colNames = lappend(colNames, makeString(pstrdup(te->resname)));
    }
    tblEntry->eref = makeAlias(pstrdup(aliasName), colNames);
    // Doesn't seem like this will have any side-effects (at this stage at least)
    tblEntry->inFromCl = false;
    tblEntry->rtekind = RTE_SUBQUERY;
    tblEntry->subquery = subQuery;
    return tblEntry;
}

Query *traceProvMakeNestedQuery(Query *base, TraceProvParseContext *context){
    Query *modified = cloneQueryForTP(base);
    ListCell *targetEntryCursor;
    foreach(targetEntryCursor, base->targetList){
        TargetEntry *baseTargetEntry = (TargetEntry *)lfirst(targetEntryCursor);
        modified->targetList = lappend(modified->targetList, makeTargetEntry(
            (Expr*)makeVarFromTargetEntry(1, baseTargetEntry),
            baseTargetEntry->resno,
            (baseTargetEntry->resname == NULL ? NULL : pstrdup(baseTargetEntry->resname)),
            false
        ));
    }
    RangeTblEntry *rte = rangeTableEntryFromSubquery(base, context);
    rte->inFromCl = true;
    modified->rtable = list_make1(rte);
    RangeTblRef *rtr = makeNode(RangeTblRef);
    rtr->rtindex = 1;
    modified->jointree = makeFromExpr(list_make1(rtr), NULL);
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

List *traceProvAggregateOnSet(
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
    traceprovAggregateRewrite(
        addedTargets,
        &aggregated,
        context,
        aggregatedInParent 
    );
    query->hasAggs = true;
    query->targetList = traceProvAppendTargets(aggregated, query->targetList);
    return aggregated;
}

bool traceProvFindIntList(List *inList, int toFind){
    ListCell *listCursor;
    foreach(listCursor, inList){
        if(lfirst_int(listCursor) == toFind) return true;
    }
    return false;
}

Node *getFunctionCallNode(const char *funcName, List *argVars){
    ParseState *dummyParseState = make_parsestate(NULL);
    List *funcNameList = list_make1(makeString(pstrdup(funcName)));
    FuncCall *fc = makeFuncCall(funcNameList, argVars, COERCE_EXPLICIT_CALL, -1);
    Node *fcNode =  ParseFuncOrColumn(
        dummyParseState,
        funcNameList,
        argVars,
        NULL,
        fc,
        false,
        fc->location
    );
    free_parsestate(dummyParseState); 
    return fcNode;
}

void traceprovAggregateRewrite(
    const List *targetEntriesToLog,
    List **pCreatedTargets,
    TraceProvParseContext *tpContext,
    bool parentHasAggs
){
    // Need to add the exprs from the targets.
    ListCell *targetEntryCursor;
    TraceProvLayerNumber layerNumber = tpParseGetLayerNumber(tpContext);
    // Need to also add the layer number (the first argument)
    Node * layerNumberConst = (Node *) makeConst(
        INT4OID, 
        -1, 
        InvalidOid,
        sizeof(int32),
        Int32GetDatum(layerNumber), 
        false,
        true
    );
    // The layer number is the first argument.
    List *argVars = list_make1(layerNumberConst);
    List *entries = NIL;
    List *childGraphs = NIL;
    foreach(targetEntryCursor, targetEntriesToLog){
        TraceProvEntry *tpEntry = tpResolveEntry(((TraceProvTarget *)lfirst(targetEntryCursor)), &childGraphs, &argVars);
        entries = lappend(entries, tpEntry);
    }

    // Need to make the func exprn.
    // Postgres' parser has all the logic already to determine functions,
    // and even adding type castes when types can be implicitly converted (like int->bigint)
    // So, the logic is just reused by making a fake parse state and then just calling Postgres'
    // parser on that.
    // TODO: Think about mixed cases (say, first three are pointers, next two are ints.)
    // In this case, we might be able to reuse the pointers.
    // TODO: Re-use pointers, rather than relogging them.
    Node *funcCallNode = getFunctionCallNode( parentHasAggs ? TRACEPROV_AGG_OFFSETS_FUNC_NAME : TRACEPROV_AGG_FUNC_NAME, argVars);
    *pCreatedTargets = list_make1(
        makeTraceProvTarget(
            true,
            makeTargetEntry(
                (Expr*) funcCallNode,
                0,
                pstrdup("mapped_agg"),
                false
            ),
            makeTraceProvDependency(
                layerNumber,
                childGraphs,
                entries
            ),
            0,
            false  
        )
    );
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
List *traceProvPropagateChildTargets(List *childTargets, Index rteIndex){
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
        targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(tpTarget->isPointer, newTarget, tpTarget->graph, tpTarget->setNumber, tpTarget->isSetPointer));
    }
    return targetsToAdd;
}

Datum
dummy_pg_get_function_sqlbody(PG_FUNCTION_ARGS)
{
	Oid			funcid = PG_GETARG_OID(0);
	StringInfoData buf;
	HeapTuple	proctup;
	bool		isnull;

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


// Get str representation of the query.
// This, first, makes a function out of the query.
// Then, looks at the SQL body of the function.
// It just, then, calls the existing postgres utility to parse back.
char *traceprovParseBackQuery(Query *query){
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
        (Node*)query,
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
        NIL,	/* parameterDefaults */
        PointerGetDatum(NULL), /* trftypes */
        PointerGetDatum(NULL), /* proconfig */
        InvalidOid,	/* prosupport */
        1.0,	/* procost */
        0.0	/* prorows */
    );
    CommandCounterIncrement();
    text *response = (DatumGetTextP(DirectFunctionCall1(dummy_pg_get_function_sqlbody, ObjectIdGetDatum(created.objectId))));
    char *str = text_to_cstring(response);
    elog(INFO, "parsed back: %s", str);
    return str;
}