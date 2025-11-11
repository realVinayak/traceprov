/*
* main rewriter entry point for TraceProv.
*/

#include <stdarg.h>
#include "c.h"
#include "postgres.h"
#include "optimizer/planner.h"
#include "fmgr.h"
#include "access/relation.h"
#include "access/htup.h"
#include "catalog/pg_index.h"
#include "utils/syscache.h"
#include "nodes/makefuncs.h"
#include "utils/rel.h"
#include "traceprov_parse_context.h"
#include "parser/parse_node.h"
#include "parser/parse_func.h"
#include "nodes/pg_list.h"
#include "access/tupdesc.h"
#include "utils/guc.h"
#include "nodes/print.h"
#include "nodes/nodes.h"
#include "catalog/pg_operator.h"
#include "parser/parse_oper.h"
#include "nodes/nodeFuncs.h"

PG_MODULE_MAGIC;

// The function name to use, for aggregation over simple primary keys.
#define TRACEPROV_AGG_FUNC_NAME "traceprov_agg_key_parallel"
#define TRACEPROV_MARK_LATER_FUNC_NAME "mark_later"
// in cases where aggregate is nested, we need to get the offsets directly.
#define TRACEPROV_AGG_OFFSETS_FUNC_NAME "traceprov_agg_key_parallel_offset"

typedef struct TraceProvTarget {
    bool isPointer;
    TargetEntry *targetEntry;
    TraceProvDependency *graph;
} TraceProvTarget;

TraceProvTarget *makeTraceProvTarget(
    bool isPointer, 
    TargetEntry *targetEntry,
    TraceProvDependency *dependency
){
    TraceProvTarget *tpTarget = palloc0_object(TraceProvTarget);
    tpTarget->isPointer = isPointer;
    tpTarget->targetEntry = targetEntry;
    tpTarget->graph = dependency;
    return tpTarget;
}

PlannedStmt *traceprov_rewriter_driver(
    Query *, 
    const char *,
    int,
	ParamListInfo
);

PlannedStmt *traceprov_rewriter(
    Query *, 
    const char *,
    int,
	ParamListInfo
);

PlannedStmt *traceprov_set_test(
    Query *,
    const char *,
    int,
	ParamListInfo
);

static Query* performTraceProvRewrite(
    Query *, 
    List **,
    TraceProvParseContext *,
    bool
);

// Performs rewrite on an RTE.
// Doesn't return a new query (rte is edited-in-place)
static void rteRewrite(
    RangeTblEntry *, 
    List **, 
    Index,
    TraceProvParseContext *,
    bool
);

/*
 * Rewrites a query that has aggregation.
 * The input target entries become the entries that we log.
*/
static void traceprovAggregateRewrite(
    Query *, List *, List **, TraceProvParseContext *, bool
);

static Query *cloneQueryForTP(const Query *);

// Appends a resjunk to end.
// Appends a non-resjunk to just before the first resjunk.
static List *appendAtResJunk(List *, TargetEntry *);

static Node *getFunctionCallNode(const char *, List *);

static Query *addNestedQuery(
    Query *, 
    List *,
    TraceProvParseContext *
);

static void adjustJoinAliasVars(List *, List *, List *, int, List **, List **);

void _PG_init(){
    planner_hook = traceprov_rewriter_driver;
}

void _PG_fini(){
    planner_hook = NULL;
}

PlannedStmt *traceprov_rewriter_driver(
    Query *parse, 
    const char *query_string,
    int cursorOptions,
	ParamListInfo boundParams
){
    // Scan the input str for ticker, and only then perform rewrites, to not mess with
    // other queries.
    bool is_traceprov_set_query = strstr(query_string, TRACEPROV_SET_TICKER) != NULL;
    if (parse->setOperations && is_traceprov_set_query){
        return traceprov_set_test(parse, query_string, cursorOptions, boundParams);
    }
    bool is_traceprov_query = strstr(query_string, TRACEPROV_TICKER) != NULL;
    if (is_traceprov_query){
        return traceprov_rewriter(parse, query_string, cursorOptions, boundParams);
    }
    return standard_planner(parse, query_string, cursorOptions, boundParams);
}

PlannedStmt *traceprov_rewriter(
    Query *parse, 
    const char *query_string,
    int cursorOptions,
	ParamListInfo boundParams
){
    TraceProvParseContext context;
    tpParseInitializeContext(&context);
    List *topLevelTargets = NIL;
    Query *traceprovParse = performTraceProvRewrite(parse, &topLevelTargets, &context, false);
    // TODO: Is it possible that the same node may go to different places?
    // Now, need to rewrite the entire query to a subquery.
    Query *traceprovTopQuery = addNestedQuery(traceprovParse, topLevelTargets, &context);
    if (Debug_print_parse)
        elog_node_display(LOG, "traceprov parse tree", traceprovTopQuery, Debug_pretty_print);
    return standard_planner(traceprovTopQuery, query_string, cursorOptions, boundParams);
}

static RangeTblEntry *rangeTableEntryFromSubquery(Query *subQuery, TraceProvParseContext *context){
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

List *addSubqueryToArgs(Query *subquery, TraceProvParseContext *context, const List *rootRTEList, Node **destination){
    RangeTblEntry *rte = rangeTableEntryFromSubquery(subquery, context);
    List *clonedRTEList = list_copy(rootRTEList);
    clonedRTEList = lappend(clonedRTEList, rte);
    RangeTblRef  *rtr = makeNode(RangeTblRef);
    rtr->rtindex = list_length(clonedRTEList);
    *destination = (Node*)rtr;
    return clonedRTEList;
}

/*
In all cases where there is no modification, we don't do anything special.
The idea is that it'll return either a new query (added to RTE) or null (not done anything to it.)
If is a leaf, it returns NULL.
There are 3 steps to all this mess
1. Break query up into subqueries
2. Fixup references in rtables
3. Convert INTERSECTION -> Joins and UNION -> Distincts.
There is some possibility to overlap these steps, but breaking them up simplifies implementation.
For example, technically, reference fix can be done during join construction, but whatever.
Doing it this way, also, actually makes debugging easier (since upto step 2, and to see if doing this
funky stuff increases the cost).
*/

Query *traceprov_breakup_sets(
    // Needed to check if the current is the same as the parent.
    SetOperation rootSetOpType, 
    bool rootIsAll,
    // Needed to copy over the RTEs
    List *rootRteList,
    Node *level,
    Query *baseQuery,
    TraceProvParseContext *context
){
    if (!IsA(level, RangeTblRef) && !IsA(level, SetOperationStmt)){
        elog(ERROR, "Expected either range table ref or set operation stmt. Got : %d", level->type);
    }
    // No need to do anything.
    if (IsA(level, RangeTblRef)) return NULL;
    
    SetOperationStmt *setOp = (SetOperationStmt *)level;
    bool isConsistent = setOp->all == rootIsAll && setOp->op == rootSetOpType;
    if (!isConsistent){
        elog(INFO, "Breaking up the query!");
        // In this case, we'll break the query up.
        // Temporarily, preserve the RTEs (they get adjusted later)
        Query *set_subquery = cloneQueryForTP(baseQuery);
        // We don't bother doing a deep copu over rtables here.
        // Since a rtable entry won't exist in more than 1 rtable,
        // we don't need to do a deep copy of rtables.
        set_subquery->rtable = list_copy(baseQuery->rtable);
        // Apparently this gets set to empty list, when there are set operations.
        set_subquery->jointree = makeFromExpr(NIL, NULL);
        // Here, the types must be same that came from the corresponding set operations.
        // So, here, need to look at the set operations.
        // TODO: Test how casting may affect things (especially during union / intersect)
        if (
            (list_length(setOp->colTypes) != list_length(setOp->colTypmods)) 
            || (list_length(setOp->colTypes) != list_length(setOp->colCollations))
        ){
            elog(ERROR, "Expected lengths to be the same!");
        }

        ListCell *colTypeCursor, *colTypModCursor, *colCollationCursor;
        int idx = 0;
        List *set_subquery_targetlist = NIL;
        forthree(colTypeCursor, setOp->colTypes, colTypModCursor, setOp->colTypmods, colCollationCursor, setOp->colCollations){
            idx++;
            Var *var = makeVar(
                1, 
                idx, 
                lfirst_oid(colTypeCursor),
                lfirst_int(colTypModCursor),
                lfirst_oid(colCollationCursor),
                0
            );
            TargetEntry *target = makeTargetEntry(
                (Expr*)var,
                idx,
                tpParseGetUniqueAlias(context),
                false
            );
            set_subquery_targetlist = lappend(set_subquery_targetlist, target);
        }
        set_subquery->targetList = set_subquery_targetlist;
        // Make the current setup root of the setop that follows.
        set_subquery->setOperations = (Node*)setOp;
        if(traceprov_breakup_sets(setOp->op, setOp->all, set_subquery->rtable, (Node*)setOp, set_subquery, context)){
            elog(ERROR, "Didn't expect recursive call to breakup sets to ever return a value!");
        }
        return set_subquery;
    }
    // In this case, we'll need to check the left and the right.
    // Possibly, we'll need to append the returned result into the rtables
    // and use subquery refs.
    Query *largQuery = traceprov_breakup_sets(rootSetOpType, rootIsAll, rootRteList, setOp->larg, baseQuery, context);
    Query *rargQuery = traceprov_breakup_sets(rootSetOpType, rootIsAll, rootRteList, setOp->rarg, baseQuery, context);

    List *newRTEList = list_copy(rootRteList);
    if (largQuery != NULL){
        // In this case, (same for rargquery), need to add the returned query to the RTE list, and replace the reference of the larg
        // to the one in CTE. This finishes the break up, of the query.
        newRTEList = addSubqueryToArgs(largQuery, context, newRTEList, &setOp->larg);
    }
    if (rargQuery != NULL){
        newRTEList = addSubqueryToArgs(rargQuery, context, newRTEList, &setOp->rarg);
    }
    baseQuery->rtable = newRTEList;
    return NULL;
}

List *traceprovFindUsedRefs(Node *level){
    if (level == NULL){
        // Nothing to do.
        return NIL;
    }
    if (!IsA(level, RangeTblRef) && !IsA(level, SetOperationStmt)){
        elog(ERROR, "Expected either range table ref or set operation stmt. Got : %d", level->type);
    }
    // No need to do anything.
    if (IsA(level, RangeTblRef)) return list_make1(level);
    SetOperationStmt *setOp = (SetOperationStmt *)level;
    return list_concat_copy(traceprovFindUsedRefs(setOp->larg), traceprovFindUsedRefs(setOp->rarg));
}

// Fixes up references.
// It is possible that rtes that are no longer needed appear in the query.
// To check for that, recursively, go through the set operation tree, find refs that appear, and only choose those refs.
void traceprov_fixup_references(Query* query){
    List *usedReferences = traceprovFindUsedRefs(query->setOperations);
    List *newRTEList = NIL;
    ListCell *usedReferenceCursor = NULL;

    if (usedReferences != NIL){
        foreach(usedReferenceCursor, usedReferences){
            RangeTblRef *ref = (RangeTblRef *)lfirst(usedReferenceCursor);
            Node *correspondingRTE = list_nth(query->rtable, ((ref)->rtindex - 1));
            newRTEList = lappend(newRTEList, correspondingRTE);
            (ref)->rtindex = list_length(newRTEList);
        }
    }else{
        newRTEList = query->rtable;
    }

    ListCell *rteCursor;
    foreach(rteCursor, newRTEList){
        RangeTblEntry *entry = (RangeTblEntry *)lfirst(rteCursor);
        if (entry->rtekind == RTE_SUBQUERY){
            traceprov_fixup_references(entry->subquery);
        }
    }
    query->rtable = newRTEList;
}

// These functions only need to care about perform a specific type of rewrite, since all the other ones are handled
// by putting them in a different table.
static void union_to_union_all(Node *node){
    // UNION -> UNION ALL
    if (!IsA(node, SetOperationStmt)) return;

    SetOperationStmt *stmt = (SetOperationStmt *)node;
    if (stmt->op != SETOP_UNION) elog(ERROR, "Expected node to be of union!, got: %d", stmt->op);
    stmt->all = true;
    stmt->op = SETOP_UNION;
    union_to_union_all(stmt->larg);
    union_to_union_all(stmt->rarg);
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

static Node *
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

Query *handleIntersect(Query *base){
    // Here, need to replace the intersect with a join.
    // For each table pair present, need to construct the join, and the join tree.
    // Basically, all the intersects within this block are consumed into a join.
    // It doesn't matter what join order is used (optimizer might reorder it anyways)
    // Simplest order is the one in RTElist, so it is directly used.
    const List *usedReferences = traceprovFindUsedRefs(base->setOperations);
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
            joinExpr->quals = createEqualityCondition(firstRTE->subquery->targetList, nextRTE->subquery->targetList, 1, 2, false);
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
            nextJoinExpr->quals = createEqualityCondition(firstRTE->subquery->targetList, nextRTE->subquery->targetList, 1, i, false);
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

Query *traceprov_rewrite_sets_to_joins(Query *base, TraceProvParseContext *context){
    // Performs following rewrites
    // UNION -> UNION ALL + Distinct
    // INTERSECT ALL -> JOIN
    // INTERSECT -> JOIN + DISTINCT

    // Nothing to do.
    if (base->setOperations == NULL) return base;
    if (!IsA(base->setOperations, SetOperationStmt))
        elog(ERROR, "Expected top level set operation to be SetOperationStmt");

    SetOperationStmt *stmt = (SetOperationStmt*)base->setOperations;
    bool wasAll = stmt->all;

    Query *modified = NULL;
    if (stmt->op == SETOP_UNION) {
        union_to_union_all((Node*)stmt);
        if (!wasAll){
            // Here, modified needs to, actually, needs to refer to a subquery.
            // For intersect, this is not needed.
            modified = cloneQueryForTP(base);
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
        }else{
            modified = base;
        }
    } else if (stmt->op == SETOP_INTERSECT){
        modified = handleIntersect(base);
        // Remove the set ops (but only if it is intersect)
        modified->setOperations = NULL;
    }

    if (!wasAll){
        elog(INFO, "Trying to make a distinct in setop!");
        // Need to add a distinct operation.
        // Here, actually, group-by is used (rather than distinct), because that's what traceprov will use.
        if (list_length(stmt->groupClauses) == 0){
            elog(ERROR, "Expected length of group clauses to defined, when not all!");
        }
        if (list_length(stmt->groupClauses) != list_length(modified->targetList)){
            // TODO: Disable this, when dealing with provenance..
            elog(ERROR, "Expected the length of group clauses to match target list");
        }
        if (list_length(modified->groupClause) != 0){
            elog(INFO, "Expected no aggregates already, in the modified group clause");
        }
        modified->groupClause = list_copy_deep(stmt->groupClauses);
        zip_target_sortgroupclause(modified->groupClause, modified->targetList);
        // TODO: Enable this when adding traceprov?
        // modified->hasAggs = true;
    }
    // Now, iterate through the RTE and do this recursively.
    ListCell *rteCursor = NULL;
    foreach(rteCursor, modified->rtable){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCursor);
        if (rte->rtekind == RTE_SUBQUERY){
            rte->subquery = traceprov_rewrite_sets_to_joins(rte->subquery, context);
        }
    }
    return modified;
}

Query *traceprov_set_rewriter(Query *initial){
    TraceProvParseContext context;
    tpParseInitializeContext(&context);
    SetOperationStmt *stmt = (SetOperationStmt *)initial->setOperations;
    if(traceprov_breakup_sets(
        stmt->op,
        stmt->all,
        list_copy(initial->rtable),
        (Node*)stmt,
        initial,
        &context
    ) != NULL){
        elog(ERROR, "expected top-level call to return no query!");
    }
    traceprov_fixup_references(initial);
    return traceprov_rewrite_sets_to_joins(initial, &context);
}

PlannedStmt *traceprov_set_test(
    Query *parse,
    const char *query_string,
    int cursorOptions,
	ParamListInfo boundParams
){
    Query *newQuery = traceprov_set_rewriter(parse);
    if (Debug_print_parse)
        elog_node_display(LOG, "traceprov set tree", newQuery, Debug_pretty_print);
    return standard_planner(newQuery, query_string, cursorOptions, boundParams);
}


// Recursively perform the traceprov rewrite.
Query * performTraceProvRewrite(
    Query *parse, 
    List **addedTargets,
    TraceProvParseContext *tpContext,
    bool parentHasAggs
){

    List *targetsToAdd = NIL;
    // We need to store what are the targets for each rte. Then, when we walk through the join tree,
    // we'll need to add these attributes to the joinaliasvars. However, interestingly, things seem to work without changing the joinaliasvar??
    // But, eh, they are added anyways (maybe there's a bug downstream that occurs....)
    List *targetsPerRTE = NIL;

    ListCell *rteCell;
    foreach(rteCell, parse->rtable){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCell);
        List *rteTargets = NIL;
        // TODO: hasAggs needs to also include distinct?
        rteRewrite(rte, &rteTargets, foreach_current_index(rteCell)+1, tpContext, parse->hasAggs);
        targetsToAdd = list_concat(targetsToAdd, rteTargets);
        targetsPerRTE = lappend(targetsPerRTE, rteTargets);
    }

    if (parse->hasAggs && parse->setOperations != NULL){
        elog(ERROR, "Didn't expect aggregation and set operations to both set!");
    }

    if (parse->hasAggs){
        // We're in an aggregation.
        // In this case, use all the generated targets, and log them, and generate pointers.
        // We'll also have to nest the entire query in a subquery block (only if we're at the top level)
        // So, it is actually two functions. The nesting occurs at the top level (since that is the only place mark is explicitly needed)
        traceprovAggregateRewrite(
            parse,
            targetsToAdd,
            &targetsToAdd,
            tpContext,
            parentHasAggs
        );
    }

    // We don't care about the top-level returned join alias vars.
    adjustJoinAliasVars(targetsPerRTE, parse->jointree->fromlist, parse->rtable, -1, NULL, NULL);

    // Now, need to recursively go through the join tree and adjust the joinaliasvars
    // In this case, simply extend the target list.
    ListCell *targetEntryCursor;

    foreach(targetEntryCursor, targetsToAdd){
        // Add target entry to the parse->targetlist.
        // TODO: Handle the case where target is pointer?
        TargetEntry *target = ((TraceProvTarget *)lfirst(targetEntryCursor))->targetEntry;
        // Here is an ugly case.
        // It is possible that the attributes we're grouping over don't appear as resjunk.
        // In that case, we'll need to adjust the references in the sort refs.
        parse->targetList = appendAtResJunk(parse->targetList, target);
    }

    if (addedTargets) *addedTargets = targetsToAdd;
    return parse;
}

// Get the primary keys for a single rte (merged during recursive caller)
void rteRewrite(
    RangeTblEntry *rte, 
    List **addedTargets, 
    Index rteIndex, 
    TraceProvParseContext *tpContext,
    bool parentHasAggs
){
    List *targetsToAdd = NIL;

    if (rte->rtekind == RTE_RELATION){
        List *indexList;
        Relation rel;
        ListCell *indexoidscan;

        // Base case.
        rel = relation_open(rte->relid, AccessShareLock);
        indexList = RelationGetIndexList(rel);
        TupleDesc tupdesc = rel->rd_att;
        foreach(indexoidscan, indexList){
            Oid indexoid = lfirst_oid(indexoidscan);
            HeapTuple indexTuple;
            Form_pg_index indexStruct;

            indexTuple = SearchSysCache(INDEXRELID, ObjectIdGetDatum(indexoid), 0, 0, 0);
            if (!HeapTupleIsValid(indexTuple))
                elog(ERROR, "cache lookup failed for index %u", indexoid);
            indexStruct = (Form_pg_index) GETSTRUCT(indexTuple);

            if (indexStruct->indisprimary){
                // Add these column names to the query.
                for (int i = 0; i < indexStruct->indnatts; i++){
                    const int16 indexAttrId = indexStruct->indkey.values[i];
                    FormData_pg_attribute *attr = TupleDescAttr(tupdesc, indexAttrId-1);
                    const char *attrName = NameStr(attr->attname);
                    elog(INFO, "Making clone of %s", attrName);
                    char *targetName = psprintf("tp_%s", attrName);

                    const Var *newVar = makeVar(
                        rteIndex, 
                        indexAttrId, 
                        attr->atttypid, 
                        attr->atttypmod, 
                        attr->attcollation,
                        0
                    );
                    // This is a dummy entry for now.
                    // This is needed to propagate the resorig* fields.
                    TargetEntry *newTargetEntry = makeTargetEntry((Expr*)newVar, indexAttrId, targetName, false);
                    newTargetEntry->resorigcol = indexAttrId;
                    newTargetEntry->resorigtbl = rte->relid;
                    // This is the base case, so that's why the isPointer is false;
                    targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(false, newTargetEntry, NULL));
                }
            }
            ReleaseSysCache(indexTuple);
        }
        relation_close(rel, AccessShareLock);
    } else if (rte->rtekind == RTE_SUBQUERY){
        List *childTargets = NIL;
        // All the next queries aren't the root.
        rte->subquery = performTraceProvRewrite(rte->subquery, &childTargets, tpContext, parentHasAggs);
        // In this case, need to convert the targets into vars.
        // Basically, the targets of the child layer become the vars for this layer.
        // These vars then get converted into new targets.
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
            targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(tpTarget->isPointer, newTarget, tpTarget->graph));
            rte->eref->colnames = lappend(rte->eref->colnames, makeString(childTarget->resname));
        }
    }

    *addedTargets = targetsToAdd;
}

void traceprovAggregateRewrite(
    Query *parse, 
    List *targetEntriesToLog, 
    List **pCreatedTargets,
    TraceProvParseContext *tpContext,
    bool parentHasAggs
){
    Assert(parse->hasAggs);
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
        const TraceProvTarget *tpTarget = ((TraceProvTarget *)lfirst(targetEntryCursor));
        const TargetEntry *target = tpTarget->targetEntry;
        argVars = lappend(argVars, target->expr);
        TraceProvEntry *tpEntry = makeTraceProvEntry();
        if (tpTarget->graph != NULL){
            if (target->resorigtbl != InvalidOid || target->resorigcol != 0){
                elog(ERROR, "Expected no table info to for the pointer node");
            }
            childGraphs = lappend(childGraphs, tpTarget->graph);
            tpEntry->kind = TP_ENTRY_KIND_POINTER;
        }else{
            tpEntry->kind = TP_ENTRY_KIND_BASE_RELATION;
            tpEntry->relId = target->resorigtbl;
            tpEntry->attr = target->resno;
        }
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
            )   
        )
    );
}

Node *getFunctionCallNode(const char *funcName, List *argVars){
    ParseState *dummyParseState = make_parsestate(NULL);
    List *funcNameList = list_make1(makeString(pstrdup(funcName)));
    FuncCall *fc = makeFuncCall(funcNameList, argVars, COERCE_EXPLICIT_CALL, -1);
    Node *fcNode = ParseFuncOrColumn(
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

Query *addNestedQuery(
    Query *base, 
    List *targets,
    TraceProvParseContext *context
){
    List *pointerTargets = NIL;
    ListCell *targetEntryCursor;
    foreach(targetEntryCursor, targets){
        const TraceProvTarget *currentTarget = ((TraceProvTarget *)lfirst(targetEntryCursor));
        if (currentTarget->isPointer){
            // Varno will be 1, because it is the only table.
            pointerTargets = lappend(pointerTargets, makeVarFromTargetEntry(1, currentTarget->targetEntry));
        }
        if (currentTarget->graph){
            traceprovPrintDependency(currentTarget->graph);
            serializeTraceProvDepedency(currentTarget->graph);
            deserializeTraceProvDependency();
        }
    }
    if (list_length(pointerTargets) == 0){
        // No pointers, no need to call the mark function.
        // Return the original query in this case.
        return base;
    }

    // Make the new table.
    RangeTblEntry *newTable = makeNode(RangeTblEntry);
    // we don't alias the table, so this is fine not being set.
    char *aliasName = tpParseGetUniqueAlias(context);
    newTable->alias = makeAlias(aliasName, NIL);
    List *newTargetList = NIL;
    targetEntryCursor = NULL;
    List *colNames = NIL;
    foreach(targetEntryCursor, base->targetList){
        TargetEntry *te = (TargetEntry *)lfirst(targetEntryCursor);
        Var *newVar = makeVarFromTargetEntry(1, te);
        if (!te->resjunk){
            newTargetList = appendAtResJunk(
                newTargetList, 
                makeTargetEntry(
                    (Expr *)newVar,
                    // Because they are 1-indexed.
                    foreach_current_index(targetEntryCursor) + 1,
                    te->resname,
                    false
                )
            );
            colNames = lappend(colNames, makeString(pstrdup(te->resname)));
        }
    }


    newTable->eref = makeAlias(pstrdup(aliasName), colNames);
    newTable->inFromCl = true;
    newTable->rtekind = RTE_SUBQUERY;
    newTable->subquery = base;

    // Make the table ref (for join tree)
    RangeTblRef  *rtr = makeNode(RangeTblRef);
    rtr->rtindex = 1;

    FromExpr *fromExpr = makeFromExpr(
        list_make1(rtr),
        NULL
    );

    Query *targetQuery = cloneQueryForTP(base);
    targetQuery->rtable = list_make1(newTable);
    targetQuery->jointree = fromExpr;

    ListCell *pointerTarget;
    foreach(pointerTarget, pointerTargets){
        int i = 0;
        Node *mark_later_func = getFunctionCallNode(TRACEPROV_MARK_LATER_FUNC_NAME, list_make1(lfirst(pointerTarget)));
        newTargetList = lappend(newTargetList, makeTargetEntry(
            (Expr *)mark_later_func,
            list_length(newTargetList) + 1,
            psprintf("marked_%d", (i++)),
            false
        ));
    }

    targetQuery->targetList = newTargetList;
    return targetQuery;
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


// Helper that also sets the resno of target entry appropriately.
List *_appendAndAdjustResno(List *inList, TargetEntry *toAdd){
    List *newList = lappend(inList, toAdd);
    toAdd->resno = list_length(newList);
    return newList;
}


List *appendAtResJunk(List *old, TargetEntry *newTe){
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

// First list is the list of targets created, for each rte.
// Second list is the list of join expressions.
// Third list is the rte list (where we'll find the current join rte and adjust the join aliasvar)
static void adjustJoinAliasVars(
    List *createdTargets, 
    List *joinExprns, 
    List *rteList,
    int parentJoinIndex,
    // OUT: new alias vars
    List **newAliasVars,
    // OUT: new alias var names
    List **newAliasNames
){
    if (parentJoinIndex == 0 || (parentJoinIndex < 0 && parentJoinIndex != -1)){
        elog(ERROR, "expected the parent join index to always be set to either -1 or > 0!");
    }
    ListCell *rteCell;
    List *createdAliases = NIL;
    List *createdAliasNames = NIL;
    foreach(rteCell, joinExprns){
        Node *targetNode = (Node *)lfirst(rteCell);
        if (IsA(targetNode, JoinExpr)){
            JoinExpr *joinExpr = (JoinExpr *)targetNode;
            List *leftAlias = NIL, *rightAlias = NIL, *leftAliasNames = NIL, *rightAliasNames = NIL;
            // Get the aliases from left side
            adjustJoinAliasVars(createdTargets, list_make1(joinExpr->larg), rteList, joinExpr->rtindex, &leftAlias, &leftAliasNames);
            // Get the alias from the right side.
            adjustJoinAliasVars(createdTargets, list_make1(joinExpr->rarg), rteList, joinExpr->rtindex, &rightAlias, &rightAliasNames);
            List *combinedAlias = list_concat_copy(leftAlias, rightAlias);
            List *combinedAliasNames = list_concat_copy(leftAliasNames, rightAliasNames);
            RangeTblEntry *rte = list_nth(rteList, joinExpr->rtindex - 1);
            rte->joinaliasvars = combinedAlias;
            rte->eref->colnames = combinedAliasNames;
            createdAliases = list_concat_copy(createdAliases, combinedAlias);
            createdAliasNames = list_concat_copy(createdAliasNames, combinedAliasNames);
        } else if (IsA(targetNode, RangeTblRef)){
            // This is, basically, the base case.
            // Here, need to first search all the 
            RangeTblRef *rangeTableRef = (RangeTblRef *)targetNode;
            // Find the previous aliases of this, first.
            // it is possible that we're not in a join tree at all
            // in this case, we obviously won't find previous aliases.
            // this can be checked by looking at parentJoinIndex (it'll be -1 otherwise.)
            if (parentJoinIndex > 0){
                RangeTblEntry *rte = list_nth(rteList, parentJoinIndex - 1);
                List *parentJoinAliasVars = rte->joinaliasvars;
                List *varsForCurrentRef = NIL;
                List *varNamesForCurrentRef = NIL;
                ListCell *cursor;
                foreach(cursor, parentJoinAliasVars){
                    const Node *varCell = (Node *)lfirst(cursor);
                    if (!IsA(varCell, Var)){
                        elog(ERROR, "Expected the entries of join alias vars to be all Vars");
                    }
                    Var *aliasVar = (Var *)varCell;
                    if (aliasVar->varno == rangeTableRef->rtindex){
                        varsForCurrentRef = lappend(varsForCurrentRef, aliasVar);
                        varNamesForCurrentRef = lappend(varNamesForCurrentRef, list_nth(rte->eref->colnames, foreach_current_index(cursor)));
                    }
                }

                // The below assertion is skipped because there can be some cases where all the columns have been
                // removed via USING. In that case, it is possible that there are no alias vars for this table.
                // if (list_length(varsForCurrentRef) == 0){
                //     elog(ERROR, "expected to find some alias vars for the current range table");
                // }

                // Need to append newly created vars (from the createdTargets, finally)
                List *rangeCreatedTargets = list_nth(createdTargets, rangeTableRef->rtindex - 1);
                cursor = NULL;
                foreach(cursor, rangeCreatedTargets){
                    const TraceProvTarget *target = ((TraceProvTarget *)lfirst(cursor));
                    if (!IsA(target->targetEntry, TargetEntry)){
                        elog(ERROR, "Expected the entries of created targets to be all targets");
                    }
                    varsForCurrentRef = lappend(varsForCurrentRef, makeVarFromTargetEntry(rangeTableRef->rtindex, target->targetEntry));
                    varNamesForCurrentRef = lappend(varNamesForCurrentRef, makeString(target->targetEntry->resname));
                }
                createdAliases = list_concat_copy(createdAliases, varsForCurrentRef);
                createdAliasNames = list_concat_copy(createdAliasNames, varNamesForCurrentRef);
            }
        } else {
            elog(ERROR, "Got unexpected node type, during adjusting!");
        }
    }
    if (newAliasVars){
        *newAliasVars = createdAliases;
    }
    if (newAliasNames){
        *newAliasNames = createdAliasNames;
    }
}