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

PG_MODULE_MAGIC;

/*
 The functions (for traceprov) can be looked up in the following order:
 1. Check if the function exists with the exact argument types.
 2. Check if the function exists with arguments that can be casted (int -> bigint)
 3. Check if a variadic function exists, with castable arguments.

 It is technically possible that casting is worse than variadic functions. That is one
 place where cost-based optimizer will be beneficial.
*/

// Search the pg proc for this function.
// Currently, it is hard coded, to look for this function.
// A better way will be to have some find of prefix, which gets
// determined during compile time. It can be explicitly set to some preferred value.
#define TRACEPROV_AGG_NAME 'traceprov_agg_key_parallel'

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

void recursiveAddPKs(
    Query *, 
    List **
);

void getPKSet(RangeTblEntry *, List **, Index);

void _PG_init(){
    planner_hook = traceprov_rewriter_driver;
}

void _PG_fini(){
    planner_hook = NULL;
}

#define TRACEPROV_TICKER "/*(traceprov)*/"

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
    recursiveAddPKs(parse, NULL);
    return standard_planner(parse, query_string, cursorOptions, boundParams);
}

// Recursively add primary keys.
// TODO: Only add them when they aren't present?
void recursiveAddPKs(
    Query *parse, 
    List **addedTargets
){

    List *targetsToAdd = NIL;

    ListCell *rteCell;
    foreach(rteCell, parse->rtable){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCell);
        List *rteTargets = NIL;
        getPKSet(rte, &rteTargets, foreach_current_index(rteCell)+1);
        targetsToAdd = list_concat(targetsToAdd, rteTargets);
        ListCell *targetEntryCursor;
        foreach(targetEntryCursor, rteTargets){
            // Add the colname to ref here.
            const TargetEntry *currentTarget = (TargetEntry *)lfirst(targetEntryCursor);
            rte->eref->colnames = lappend(rte->eref->colnames,  makeString(currentTarget->resname));
        }
    }

    ListCell *targetEntryCursor;

    foreach(targetEntryCursor, targetsToAdd){
        // Add target entry to the parse->targetlist.
        TargetEntry *target = lfirst(targetEntryCursor);
        target->resno = list_length(parse->targetList) + 1;      
        parse->targetList = lappend(parse->targetList, target);
    }

    if (addedTargets) *addedTargets = targetsToAdd;
}

// Get the primary keys for a single rte (merged during recursive caller)
void getPKSet(RangeTblEntry *rte, List **addedTargets, Index rteIndex){
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
                    FormData_pg_attribute *attr = &tupdesc->attrs[indexAttrId-1];
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
                    targetsToAdd = lappend(targetsToAdd, newTargetEntry);
                }
            }
            ReleaseSysCache(indexTuple);
        }
        relation_close(rel, AccessShareLock);
    } else if (rte->rtekind == RTE_SUBQUERY){
        List *childTargets = NIL;
        recursiveAddPKs(rte->subquery, &childTargets);
        // In this case, need to convert the targets into vars.
        // Basically, the targets of the child layer become the vars for this layer.
        // These vars then get converted into new targets.
        ListCell *childTargetCell;
        foreach(childTargetCell, childTargets){
            TargetEntry *childTarget = (TargetEntry *)lfirst(childTargetCell);
            const Var *newVar = makeVarFromTargetEntry(rteIndex, childTarget);
            TargetEntry *newTarget = makeTargetEntry(
                (Expr *)newVar, 0, childTarget->resname, false
            );
            newTarget->resorigcol = childTarget->resorigcol;
            newTarget->resorigtbl = childTarget->resorigtbl;
            targetsToAdd = lappend(targetsToAdd, newTarget);
        }
    }

    *addedTargets = targetsToAdd;
}