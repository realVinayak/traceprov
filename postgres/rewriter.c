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

PG_MODULE_MAGIC;

// The function name to use, for aggregation over simple primary keys.
#define TRACEPROV_AGG_NAME_FUNC_NAME "traceprov_agg_key_parallel"
#define TRACEPROV_MARK_LATER_FUNC_NAME "mark_later"

typedef struct TraceProvTarget {
    bool isPointer;
    TargetEntry *targetEntry;
} TraceProvTarget;

TraceProvTarget *makeTraceProvTarget(bool isPointer, TargetEntry *targetEntry){
    TraceProvTarget *tpTarget = palloc0_object(TraceProvTarget);
    tpTarget->isPointer = isPointer;
    tpTarget->targetEntry = targetEntry;
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

static Query* performTraceProvRewrite(
    Query *, 
    List **,
    TraceProvParseContext *
);

// Performs rewrite on an RTE.
// Doesn't return a new query (rte is edited-in-place)
static void rteRewrite(
    RangeTblEntry *, 
    List **, 
    Index,
    TraceProvParseContext *
);

/*
 * Rewrites a query that has aggregation.
 * The input target entries become the entries that we log.
*/
static void traceprovAggregateRewrite(
    Query *, List *, List **, TraceProvParseContext *
);

static Query *cloneQueryForTP(Query *);

// Appends a resjunk to end.
// Appends a non-resjunk to just before the first resjunk.
static List *appendAtResJunk(List *, TargetEntry *);

static Node *getFunctionCallNode(const char *, List *);

static Query *addNestedQuery(
    Query *, 
    List *,
    TraceProvParseContext *
);

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
    // For simplicity, only handle this case for now.
    TraceProvParseContext context;
    tpParseInitializeContext(&context);
    List *topLevelTargets = NIL;
    Query *traceprovParse = performTraceProvRewrite(parse, &topLevelTargets, &context);
    // TODO: Is it possible that the same node may go to different places?
    // TODO: Need some kind of structure that stores the path of values.
    // Now, need to rewrite the entire query to a subquery.
    Query *traceprovTopQuery = addNestedQuery(traceprovParse, topLevelTargets, &context);
    if (Debug_print_parse)
        elog_node_display(LOG, "traceprov parse tree", traceprovTopQuery, Debug_pretty_print);
    return standard_planner(traceprovTopQuery, query_string, cursorOptions, boundParams);
}
// Recursively perform the traceprov rewrite.
Query * performTraceProvRewrite(
    Query *parse, 
    List **addedTargets,
    TraceProvParseContext *tpContext
){

    List *targetsToAdd = NIL;

    ListCell *rteCell;
    foreach(rteCell, parse->rtable){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCell);
        List *rteTargets = NIL;
        rteRewrite(rte, &rteTargets, foreach_current_index(rteCell)+1, tpContext);
        targetsToAdd = list_concat(targetsToAdd, rteTargets);
        ListCell *targetEntryCursor;
        if (rte->rtekind != RTE_RELATION){
            // Seems like all the columns are included, if the RTE is relation.
            // So, technically, we don't need the columns.
            foreach(targetEntryCursor, rteTargets){
                // Add the colname to ref here.
                const TargetEntry *currentTarget = ((TraceProvTarget *)lfirst(targetEntryCursor))->targetEntry;
                rte->eref->colnames = lappend(rte->eref->colnames,  makeString(currentTarget->resname));
            }
        }
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
            tpContext
        );
    }
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
void rteRewrite(RangeTblEntry *rte, List **addedTargets, Index rteIndex, TraceProvParseContext *tpContext){
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
                    TargetEntry *newTargetEntry = makeTargetEntry((Expr*)newVar, 0, targetName, false);
                    newTargetEntry->resorigcol = indexAttrId;
                    newTargetEntry->resorigtbl = rte->relid;
                    // This is the base case, so that's why the isPointer is false;
                    targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(false, newTargetEntry));
                }
            }
            ReleaseSysCache(indexTuple);
        }
        relation_close(rel, AccessShareLock);
    } else if (rte->rtekind == RTE_SUBQUERY){
        List *childTargets = NIL;
        // All the next queries aren't the root.
        rte->subquery = performTraceProvRewrite(rte->subquery, &childTargets, tpContext);
        // In this case, need to convert the targets into vars.
        // Basically, the targets of the child layer become the vars for this layer.
        // These vars then get converted into new targets.
        ListCell *childTargetCell;
        foreach(childTargetCell, childTargets){
            TraceProvTarget *tpTarget = (TraceProvTarget *)lfirst(childTargetCell);
            TargetEntry *childTarget = tpTarget->targetEntry;
            const Var *newVar = makeVarFromTargetEntry(rteIndex, childTarget);
            TargetEntry *newTarget = makeTargetEntry(
                (Expr *)newVar, 0, childTarget->resname, false
            );
            newTarget->resorigcol = childTarget->resorigcol;
            newTarget->resorigtbl = childTarget->resorigtbl;
            targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(tpTarget->isPointer, newTarget));
        }
    }

    *addedTargets = targetsToAdd;
}

void traceprovAggregateRewrite(
    Query *parse, 
    List *targetEntriesToLog, 
    List **pCreatedTargets,
    TraceProvParseContext *tpContext
){
    Assert(parse->hasAggs);
    // Need to add the exprs from the targets.
    ListCell *targetEntryCursor;
    // Need to also add the layer number (the first argument)
    Node * layerNumberConst = (Node *) makeConst(
        INT4OID, 
        -1, 
        InvalidOid,
        sizeof(int32),
        Int32GetDatum(tpParseGetLayerNumber(tpContext)), 
        false,
        true
    );
    // The layer number is the first argument.
    List *argVars = list_make1(layerNumberConst);
    foreach(targetEntryCursor, targetEntriesToLog){
        const TargetEntry *target = ((TraceProvTarget *)lfirst(targetEntryCursor))->targetEntry;
        argVars = lappend(argVars, target->expr);
    }
    
    // Need to make the func exprn.
    // Postgres' parser has all the logic already to determine functions,
    // and even adding type castes when types can be implicitly converted (like int->bigint)
    // So, the logic is just reused by making a fake parse state and then just calling Postgres'
    // parser on that.
    // TODO: Think about mixed cases (say, first three are pointers, next two are ints.)
    // In this case, we might be able to reuse the pointers.
    // TODO: Re-use pointers, rather than relogging them.
    Node *funcCallNode = getFunctionCallNode(TRACEPROV_AGG_NAME_FUNC_NAME, argVars);
    // Add the new target.
    *pCreatedTargets = list_make1(
        makeTraceProvTarget(
            true,
            makeTargetEntry(
                (Expr*) funcCallNode,
                0,
                pstrdup("mapped_agg"),
                false
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
    }
    if (list_length(pointerTargets) == 0){
        // No pointers, no need to call the mark function.
        // Return the original query in this case.
        return base;
    }
    Node *mark_later_func = getFunctionCallNode(TRACEPROV_MARK_LATER_FUNC_NAME, pointerTargets);

    // Make the new table.
    RangeTblEntry *newTable = makeNode(RangeTblEntry);
    // we don't alias the table, so this is fine not being set.
    char *aliasName = tpParseGetUniqueAlias(context);
    newTable->alias = makeAlias(aliasName, NULL);
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

    newTargetList = lappend(newTargetList, makeTargetEntry(
        (Expr *)mark_later_func,
        list_length(newTargetList) + 1,
        pstrdup("marked"),
        false
    ));

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
    targetQuery->jointree = fromExpr,
    targetQuery->targetList = newTargetList;
    return targetQuery;
}

Query *cloneQueryForTP(Query *base){
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