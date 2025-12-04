#include "c.h"
#include "postgres.h"
#include "nodes/plannodes.h"
#include "miscadmin.h"

#include "traceprov_parse_context.h"
#include "rewriter_utils.h"

static bool
plan_walk_many(List *plans, bool (*walker)(), void *context){
    ListCell *plan;
    foreach(plan, plans){
        if (walker(lfirst_node(Plan, plan), context)) return true;
    }
    return false;
}

bool
plan_tree_walker(Plan *plan, bool (*walker)(), void *context){

    if (plan == NULL) return false;

    check_stack_depth();

    if (walker(plan, context)) return true;

    if(plan_walk_many(plan->initPlan, walker, context)) return true;
    if(plan_tree_walker(outerPlan(plan), walker, context)) return true;
    if(plan_tree_walker(innerPlan(plan), walker, context)) return true; 

    switch (nodeTag(plan))
    {
    case T_Append:
        if (plan_walk_many(((Append *)plan)->appendplans, walker, context))
            return true;
        break;
    case T_MergeAppend:
        if (plan_walk_many(((MergeAppend *)plan)->mergeplans, walker, context))
            return true;
        break;
    case T_BitmapAnd:
        if (plan_walk_many(((BitmapAnd *)plan)->bitmapplans, walker, context))
            return true;
        break;
    case T_BitmapOr:
        if (plan_walk_many(((BitmapOr *)plan)->bitmapplans, walker, context))
            return true;
        break;
    case T_SubqueryScan:
        if (plan_tree_walker(((SubqueryScan *)plan)->subplan, walker, context))
            return true;
        break;
    case T_Agg:
        if ((plan_walk_many(((Agg *) plan)->chain, walker, context)))
            return true;
        break;
    default:
        break;
    }
    return false;
}

bool plan_walker(Plan *plan, void *context){
    TraceProvParseContext *parse_context = (TraceProvParseContext *)context;
    return true;
    if (plan == NULL) return false;
    if (IsA(plan, Agg)){
        elog(INFO, "detected agg!");
        const Agg *agg_plan =(Agg *)plan;
        // Need to identify the traceprov funcs.
        ListCell *cursor;
        foreach(cursor, plan->targetlist){
            const TargetEntry *target_entry = lfirst_node(TargetEntry, cursor);
            if (!IsA(target_entry->expr, Aggref)) continue;
            const Aggref *aggref = (Aggref *)(target_entry->expr);
            if (!traceprov_find_oid_list(parse_context->properties->traceprov_funcs, aggref->aggfnoid)) continue;
            // It is a traceprov function. Need to look at the args to figure out the corresponding layer number.
            const Expr *arg = lfirst_node(TargetEntry, list_head(aggref->args))->expr;
            if (!IsA(arg, Const)){
                elog(ERROR, "Expected the first argument to be a constant!");
            }
            const Const *const_arg = (Const *)arg;
            const TraceProvLayerNumber layer_number = (TraceProvLayerNumber)DatumGetInt32(const_arg->constvalue);
            elog(INFO, "TraceProv Arg Value: %d. Method: %d", layer_number, agg_plan->aggstrategy);
            tp_add_aggregate_property(parse_context, agg_plan, layer_number);
        }
    }
    return false;
}


void walk_planned_stmt(PlannedStmt *stmt, void *context){
    if (plan_tree_walker(stmt->planTree, plan_walker, context)) return;
    if (plan_walk_many(stmt->subplans, plan_walker, context)) return;
}

void traceprov_plan_analyzer(
    PlannedStmt *stmt, 
    TraceProvDependency *traceprov_graph, 
    TraceProvParseContext *context)
{
    walk_planned_stmt(stmt, context);
}