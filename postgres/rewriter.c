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
#include "rewriter_utils.h"
#include "parser/parse_type.h"
#include "parser/analyze.h"

#include "rewriter_sets.h"
#include "rewriter_sublinks.h"

#include "access/xact.h"
#include "rewrite/rewriteManip.h"
#include "executor/executor.h"
#include "rewrite/rewriteHandler.h"

#include "plan_analyzer.h"

PG_MODULE_MAGIC;

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

Query *traceprov_add_nested_query_log(
    Query *, 
    List *,
    TraceProvParseContext *,
    TraceProvDependency **pgraph
);

static void adjustJoinAliasVars(List *, List *, List *, int, List **, List **);

void _PG_init(){
    planner_hook = traceprov_rewriter_driver;
}

void _PG_fini(){
    planner_hook = NULL;
    ExecutorStart_hook = NULL;
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
    Query *copied = copyObject(parse);
    ListCell *cursor = NULL;
    List *rewritten = QueryRewrite(copied);
    foreach(cursor, rewritten){
        if (Debug_print_parse)
            elog_node_display(LOG, "rewritten", lfirst_node(Query, cursor), Debug_pretty_print);
    }
    TraceProvParseContext context;
    tpParseInitializeContext(&context);
    // Set the root context to the parent context.
    // This simplifies some operations (since, otherwise, layer numbers can be same across branches, during sublink)
    context.root_context = &context;
    List *top_level_targets = NIL;
    Query *traceprov_parse = traceprov_perform_rewrite(parse, &top_level_targets, &context, false);
    // TODO: Is it possible that the same node may go to different places?
    // Now, need to rewrite the entire query to a subquery.
    ListCell *te_cursor;
    foreach(te_cursor, top_level_targets){
        const TraceProvTarget *currentTarget = ((TraceProvTarget *)lfirst(te_cursor));
        if (currentTarget->graph){
            traceprovPrintDependency(currentTarget->graph, &context);
        }
    }
    // Query *traceprovTopQuery = add_nested_query(traceprovParse, topLevelTargets, &context);
    TraceProvDependency *graph = NULL;
    Query *traceprov_top_query = traceprov_add_nested_query_log(traceprov_parse, top_level_targets, &context, &graph);
    if (Debug_print_parse)
        elog_node_display(LOG, "traceprov parse tree", traceprov_top_query, Debug_pretty_print);

    char *parsed_back = tracprov_parse_back_query(traceprov_top_query);
    serializeTraceProvDepedency(list_make1(graph), &context, parsed_back);
    TraceProvParseContext *dupContext = NULL;
    char *parsed_back_parsed = NULL;
    deserializeTraceProvDependency(&dupContext, &parsed_back_parsed);
    elog(INFO, "PARSED BACK (from FILE): %s", parsed_back_parsed);


    PlannedStmt *stmt = standard_planner(traceprov_top_query, query_string, cursorOptions, boundParams);
    traceprov_plan_analyzer(stmt, NULL, &context);
    return stmt;
}

List *addSubqueryToArgs(Query *subquery, TraceProvParseContext *context, const List *rootRTEList, Node **destination){
    RangeTblEntry *rte = range_table_entry_from_subquery(subquery, context, false);
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
        Query *set_subquery = traceprov_clone_query(baseQuery);
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
                tp_parse_get_unique_alias(context),
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
        IncrementVarSublevelsUp((Node*)set_subquery, 1, 1);
        return set_subquery;
    }
    // In this case, we'll need to check the left and the right.
    // Possibly, we'll need to append the returned result into the rtables
    // and use subquery refs.
    Query *largQuery = traceprov_breakup_sets(rootSetOpType, rootIsAll, rootRteList, setOp->larg, baseQuery, context);
    Query *rargQuery = traceprov_breakup_sets(rootSetOpType, rootIsAll, baseQuery->rtable, setOp->rarg, baseQuery, context);

    List *newRTEList = list_copy(baseQuery->rtable);
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

// Fixes up references.
// It is possible that rtes that are no longer needed appear in the query.
// To check for that, recursively, go through the set operation tree, find refs that appear, and only choose those refs.
void traceprov_fixup_references(Query* query){
    List *usedReferences = traceprov_find_used_refs(query->setOperations, traceProvLeafNavigator);
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

Query *traceprov_rewrite_sets_to_joins(
    Query *base, 
    TraceProvParseContext *context,
    // For UNIONs (and INTERSECTS), these columns are skipped for aggregation.
    // That is, the grouping is performed without these columns present.
    // These attributes also act the input to the aggregate function.
    List *ignoreList,
    List **createdTargets,
    bool aggregatedInParent
){
    // Performs following rewrites
    // UNION -> UNION ALL + Distinct
    // INTERSECT ALL -> JOIN
    // INTERSECT -> JOIN + DISTINCT

    // Nothing to do.
    if (base->setOperations == NULL) return base;
    if (!IsA(base->setOperations, SetOperationStmt)) elog(ERROR, "Expected top level set operation to be SetOperationStmt");
    SetOperationStmt *stmt = (SetOperationStmt*)base->setOperations;
    // Ignore except set op.
    if (stmt->op != SETOP_UNION && stmt->op != SETOP_INTERSECT) return base;
    List *flattenedAddedTargets = traceprov_flatten(ignoreList);

    bool wasAll = stmt->all;

    Query *modified = NULL;
    if (stmt->op == SETOP_UNION) {
        union_to_union_all((Node*)stmt);
        if (!wasAll){
            // Here, modified needs to, actually, needs to refer to a subquery.
            // For intersect, this is not needed.
            modified = traceprov_make_nested_query(base, context, false, false);
        }else{
            modified = base;
        }
    } else if (stmt->op == SETOP_INTERSECT){
        // modified = handleIntersect(base, ignoreList, context);
        // Remove the set ops (but only if it is intersect)
        modified = base;
	modified->setOperations = NULL;
    }

    if (!wasAll){
        List *newTargetList = NIL;
        for (int i = 0; i < list_length(modified->targetList); i++){
            if (i < (list_length(modified->targetList) - list_length(flattenedAddedTargets))){
                newTargetList = lappend(newTargetList, list_nth(modified->targetList, i));
                continue;
            }
        }
        List *aggregated = traceprov_aggregate_on_set(stmt, modified, context, aggregatedInParent, newTargetList, flattenedAddedTargets);
        if (createdTargets) *createdTargets = aggregated;
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
    return traceprov_rewrite_sets_to_joins(initial, &context, NIL, NULL, false);
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
Query *traceprov_perform_rewrite(
    Query *parse, 
    List **addedTargets,
    TraceProvParseContext *tp_context,
    bool parentHasAggs
){
    bool purePointerInParent = parentHasAggs || parse->hasAggs;
    if (parse->hasAggs && parse->setOperations != NULL){
        elog(ERROR, "Didn't expect aggregation and set operations to both set!");
    }
    
    List *rteFilter = NIL;

    if (parse->setOperations != NULL){
        // Assert that there are no res junks, if there are set operations.
        traceprov_assert_no_resjunk(parse->rtable);
        // Normalize the query here.
        SetOperationStmt *stmt = (SetOperationStmt *)parse->setOperations;
        purePointerInParent |= !stmt->all;
        if(traceprov_breakup_sets(
            stmt->op,
            stmt->all,
            list_copy(parse->rtable),
            (Node*)stmt,
            parse,
            tp_context
        ) != NULL){
            elog(ERROR, "expected top-level call to return no query!");
        }
        traceprov_fixup_references(parse);
        if (stmt->op == SETOP_EXCEPT){
            TraceProvUsedRefNavigator leftNavigator = {
                .goLeft = true,
                .goRight = false,
                .includeInternals = false
            };
            List *leftMostElement = traceprov_find_used_refs((Node*)stmt, leftNavigator);
            // There should always be just one left-most element. assert thatn.
            traceprov_assert_equal_length(list_make2(leftMostElement, list_make1(NULL)));
            const RangeTblRef *leftMostRef = (RangeTblRef *)lfirst(list_head(leftMostElement));
            rteFilter = list_make1_int(leftMostRef->rtindex);
        }
    }

    List *targets_to_add = NIL;
    // We need to store what are the targets for each rte. Then, when we walk through the join tree,
    // we'll need to add these attributes to the joinaliasvars. However, interestingly, things seem to work without changing the joinaliasvar??
    // But, eh, they are added anyways (maybe there's a bug downstream that occurs....)
    List *targets_per_rte = NIL;

    ListCell *rteCell;

    foreach(rteCell, parse->rtable){
        RangeTblEntry *rte = (RangeTblEntry *)lfirst(rteCell);
        List *rteTargets = NIL;
        if (list_length(rteFilter) == 0 || (traceprov_find_int_list(rteFilter, foreach_current_index(rteCell)+1))){
            // TODO: hasAggs needs to also include distinct?
            rteRewrite(rte, &rteTargets, foreach_current_index(rteCell)+1, tp_context, purePointerInParent);
        }else{
            rteTargets = NIL;
        }
        targets_to_add = list_concat(targets_to_add, rteTargets);
        targets_per_rte = lappend(targets_per_rte, rteTargets);
    }

    // if there are sublinks, need to go, also, go over them.
    // we log all of the matching keys, from the left side.
    if (parse->hasSubLinks){
        traceprov_rewrite_sublinks(parse, tp_context, targets_per_rte);
    }

    if (parse->hasAggs){
        // We're in an aggregation.
        // In this case, use all the generated targets, and log them, and generate pointers.
        // We'll also have to nest the entire query in a subquery block (only if we're at the top level)
        // So, it is actually two functions. The nesting occurs at the top level (since that is the only place mark is explicitly needed)
        traceprov_aggregate_rewrite(
            targets_to_add,
            &targets_to_add,
            tp_context,
            parentHasAggs,
            NULL,
            NULL
        );
    }

    if (parse->setOperations != NULL){
        if (parse->jointree->fromlist != NIL){
            elog(ERROR, "Expected from list to be empty, when set operations are present!");
        }
        // If it is a union, then need to also add numbering to the attributes.s
        // TODO: Discuss splitting the unions into different files (can be done dynamically)
        // That way, inference can be parallelized across unions.
        SetOperationStmt *setop = (SetOperationStmt *)parse->setOperations;
        if (setop->op == SETOP_UNION){
            // In this case, need to walk through the tree, and do two things:
            // 1. Make the width same (can be different, now). trivally, when pks are of different length.
            // 2. Add some counter for setop.
            // Don't need do anything in the case of intersect.
            List *unionAdjusted = traceprov_adjust_union(
                setop, targets_per_rte, parse->rtable, tp_context
            );
            targets_to_add = traceprov_flatten(unionAdjusted);
            targets_per_rte = unionAdjusted;
        } else if (setop->op == SETOP_EXCEPT){
            parse = traceprov_adjust_except(
                parse,
                targets_per_rte,
                tp_context,
                parentHasAggs,
                &targets_to_add
            );
            targets_per_rte = NIL;
        } else if (setop->op == SETOP_INTERSECT){
            targets_per_rte = traceprov_adjust_intersect(parse, targets_per_rte, tp_context);
            targets_to_add = traceprov_flatten(targets_per_rte);
        }
    }else{
        // We don't care about the top-level returned join alias vars.
        // need to recursively go through the join tree and adjust the joinaliasvars
        adjustJoinAliasVars(targets_per_rte, parse->jointree->fromlist, parse->rtable, -1, NULL, NULL);
    }

    if (parse->hasWindowFuncs){
        if (parse->setOperations != NULL)
            elog(ERROR, "Didn't expect setops to be at the same level as window funcs");
        List *window_targets = NIL;
        parse = traceprov_perform_window_rewrite(
            parse,
            tp_context,
            targets_per_rte,
            &window_targets
        );
        targets_to_add = window_targets;
    }
    // In this case, simply extend the target list.
    parse->targetList = traceprov_append_targets(targets_to_add, parse->targetList);

    List *added_from_set_ops = NIL;
    parse = traceprov_rewrite_sets_to_joins(parse, tp_context, targets_per_rte, &added_from_set_ops, parentHasAggs);
    if (added_from_set_ops != NIL){
        targets_to_add = added_from_set_ops;
    }

    if (addedTargets) *addedTargets = targets_to_add;

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
                    // Also why there's no sublinks (yet)
                    targetsToAdd = lappend(targetsToAdd, makeTraceProvTarget(false, newTargetEntry, NULL, 0, false, NIL, NULL));
                }
            }
            ReleaseSysCache(indexTuple);
        }
        relation_close(rel, AccessShareLock);
    } else if (rte->rtekind == RTE_SUBQUERY){
        List *childTargets = NIL;
        // All the next queries aren't the root.
        rte->subquery = traceprov_perform_rewrite(rte->subquery, &childTargets, tpContext, parentHasAggs);
        targetsToAdd = traceprov_propagate_child_targets(childTargets, rteIndex);
        ListCell *childTargetCell;
        // Add the created child targets to the RTE colnames.
        foreach(childTargetCell, targetsToAdd){
            TraceProvTarget *tpTarget = (TraceProvTarget *)lfirst(childTargetCell);
            TargetEntry *childTarget = tpTarget->targetEntry;
            rte->eref->colnames = lappend(rte->eref->colnames, makeString(childTarget->resname));
        }
    }
    *addedTargets = targetsToAdd;
}

Query *traceprov_add_nested_query_log(
    Query *base,
    List *targets,
    TraceProvParseContext *context,
    TraceProvDependency **pgraph
){
    if (list_length(targets) == 0)
        elog(ERROR, "Expected some traceprov targets!");

    const TraceProvLayerNumber layer_number = tp_parse_get_layer_number(context);
    ListCell *target_entry_cursor;
    List *child_graphs = NIL;
    List *entries = NIL;
    List *arg_vars = list_make1((Node *) makeConst(
        INT4OID, 
        -1, 
        InvalidOid,
        sizeof(int32),
        Int32GetDatum(layer_number), 
        false,
        true
    ));
    foreach(target_entry_cursor, targets){
        const TraceProvTarget *tp_target = (TraceProvTarget *)lfirst(target_entry_cursor);
        TraceProvEntry *tp_entry = traceprov_resolve_entry(
            tp_target,
            &child_graphs,
            NULL
        );
        entries = lappend(entries, tp_entry);
        arg_vars = lappend(arg_vars, makeVarFromTargetEntry(1, tp_target->targetEntry));
    }
    TraceProvDependency *graph = make_traceprov_dependency(
        TP_LOG,
        layer_number,
        child_graphs,
        entries
    );

    Query *nested = traceprov_make_nested_query(base, context, false, false);
    Node *traceprov_log_fcnode = traceprov_get_function_call_node(TRACEPROV_LOG_FUNC_NAME, arg_vars, NULL);
    nested->targetList = traceprov_append_at_resjunk(
        nested->targetList,
        makeTargetEntry((Expr*)traceprov_log_fcnode, 0, tp_parse_get_unique_alias(context), false)
    );

    *pgraph = graph;
    return nested;
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
