#include "rewriter_sublinks.h"
#include "nodes/pathnodes.h"
#include "rewrite/rewriteManip.h"
#include "catalog/pg_type_d.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"

static Node *rewrite_sublinks_mutator(Node *, TraceProvParseContext *);

// To a given sublink, there is a stack of provenance attributes are available
// for logging. This needs to be a stack, since inner levels can (potentially) refer to
// attributes arbitrarily up.
// The rewrite is implemented using walkers (over the expressions)
void traceprov_rewrite_sublinks(Query *query, TraceProvParseContext*context, const List *added_targets_per_rte){
    if (!query->hasSubLinks) return;
    context->parent_targets = lappend(context->parent_targets, (List*)added_targets_per_rte);

    query->targetList = (List*)rewrite_sublinks_mutator((Node*)query->targetList, context);
    query->jointree = (FromExpr*)rewrite_sublinks_mutator((Node*)query->jointree, context);
    query->havingQual = rewrite_sublinks_mutator((Node*)query->havingQual, context);
}

static Node *rewrite_sublinks_mutator(Node *node, TraceProvParseContext *context){
    // base case.
    if (node == NULL) return NULL;

    if (IsA(node, BoolExpr)){
        BoolExpr *bool_expr = (BoolExpr*)node;
        // Don't do anything it is a not.
        if (bool_expr->boolop == NOT_EXPR) return node;
    }

    if (IsA(node, SubLink)){
        SubLink *originalSublink = (SubLink *)node;
        SubLink *sublink = copyObject(originalSublink);
        TraceProvParseContext *new_context = traceprov_shallow_copy_context(context);
        List *added_targets = NIL;
        if (!IsA(sublink->subselect, Query)){
            elog(ERROR, "Expected subselect to be a query node");
        }

        const int original_target_count = list_length(((Query *)sublink->subselect)->targetList);
        Query *rewritten_subselect = traceprov_perform_rewrite(
            (Query *)sublink->subselect, 
            &added_targets,
            new_context,
            true
        );
        if ((original_target_count + list_length(added_targets)) != (list_length(rewritten_subselect->targetList))){
            elog(ERROR, "Unexpected target list addition!");
        }
        // Figure out the correlated references, and also log the provenance attributes of them.
        // This is done by going through the queue of provenance attributes, level by level.
        // We stop once we see find any match.
        List *correlated_arg_vars = NIL;
        ListCell *provenance_queue = NULL;
        List *correlated_provenance_targets = NIL;
        foreach(provenance_queue, new_context->parent_targets){
            const int var_level_id = foreach_current_index(provenance_queue) + 1;
            const List *provenance_targets_per_rte = (List*)lfirst(provenance_queue);
            const List *correlated_targets = pull_vars_of_level_ignore_sublinks((Node*)rewritten_subselect, var_level_id);

            // no correlated vars case. 
            if (list_length(correlated_targets) == 0) continue;
            ListCell *correlated_var_cursor;

            foreach(correlated_var_cursor, correlated_targets){
                const Var *correlated_var = (Var *)lfirst(correlated_var_cursor);
                List *provenance_targets = list_nth(provenance_targets_per_rte, correlated_var->varno - 1);
                ListCell *provenance_target_to_add;
                correlated_provenance_targets = provenance_targets;
                foreach(provenance_target_to_add, provenance_targets){
                    Var *outerVar = NULL;
                    const TraceProvTarget *prov_target = ((TraceProvTarget *)lfirst(provenance_target_to_add));
                    // At this point, regardless of how it is structured, it will always be a var.
                    // Only case where we have constants are still going to be seen as vars at this level.
                    if (!IsA((prov_target->targetEntry->expr), Var)){
                        elog(ERROR, "traceprov: Expected the prov target to always be a var.");
                    }
                    outerVar = (Var*)copyObject(prov_target->targetEntry->expr);
                    outerVar->varlevelsup = correlated_var->varlevelsup;
                    correlated_arg_vars = lappend(
                        correlated_arg_vars, 
                        outerVar
                    );
                }
            }
            // If we find one correlation, that's all we need.
            break;
        }
        // Log the traceprov references.
        TraceProvLayerNumber layer_number = tp_parse_get_layer_number(context->root_context);
        // Add the correlated targets (as keys), and the targets of the base query to the context.
        tp_add_sublink_map_item(new_context, correlated_provenance_targets, added_targets, layer_number);
        Node * layer_number_const = (Node *) makeConst(
            INT4OID, 
            -1, 
            InvalidOid,
            sizeof(int32),
            Int32GetDatum(layer_number), 
            false,
            true
        );
        List *arg_vars = list_make1(layer_number_const);
        ListCell *target_entry_cursor;
        foreach(target_entry_cursor, added_targets){
            TraceProvTarget *tp_target = (TraceProvTarget*)lfirst(target_entry_cursor);
            TargetEntry *base_target = tp_target->targetEntry;
            // Add all the expressions. This function call will replace the current target list.
            // This could be done, potentially, better by having yet another subquery block.
            arg_vars = lappend(arg_vars, base_target->expr);
        }
        arg_vars = list_concat(arg_vars, correlated_arg_vars);
        Node *traceprov_log_fcnode = traceprov_get_function_call_node(TRACEPROV_LOG_VOLATILE_FUNC_NAME, arg_vars);
        // Replace all the extra added traceprov targets with this newer func node.
        target_entry_cursor = NULL;
        List *original_without_targets = NIL;
        foreach(target_entry_cursor, rewritten_subselect->targetList){
            ListCell *inner_cell = NULL;
            bool found = false;
            foreach(inner_cell, added_targets){
                if (((TraceProvTarget *)lfirst(inner_cell))->targetEntry == ((TargetEntry *)lfirst(target_entry_cursor))){
                    found = true;
                    break;
                }
            }
            if (!found){
                original_without_targets = lappend(original_without_targets, lfirst(target_entry_cursor));
            }
        }
        if (list_length(original_without_targets) != (original_target_count)){
            elog(ERROR, "Got mismatching original target lengths!");
        }
        // Now, append the newly created function call node, to the target list.
        TargetEntry *log_target_entry = makeTargetEntry((Expr*)traceprov_log_fcnode, 0, tp_parse_get_unique_alias(context->root_context), false);
        rewritten_subselect->targetList = traceProvAppendAtResJunk(original_without_targets, log_target_entry);

        if (sublink->subLinkType != EXISTS_SUBLINK || true){
            Query *subselect_wrapper = traceprov_clone_query(rewritten_subselect);
            subselect_wrapper->targetList = NIL;

            foreach(target_entry_cursor, original_without_targets){
                TargetEntry *base_target_entry = (TargetEntry *)lfirst(target_entry_cursor);
                // Don't add the log entry (since that'll violate the query semantics.)
                subselect_wrapper->targetList = lappend(
                    subselect_wrapper->targetList, 
                    makeTargetEntry(
                        (Expr*)makeVarFromTargetEntry(1, base_target_entry),
                        base_target_entry->resno,
                        (base_target_entry->resname == NULL ? NULL : pstrdup(base_target_entry->resname)),
                        false
                    )
                );
            }
            RangeTblEntry *rte = rangeTableEntryFromSubquery(rewritten_subselect, context);
            rte->inFromCl = true;
            subselect_wrapper->rtable = list_make1(rte);
            RangeTblRef *rtr = makeNode(RangeTblRef);
            rtr->rtindex = 1;
            subselect_wrapper->jointree = makeFromExpr(list_make1(rtr), NULL);
            // Move all the vars one level down.
            // This will also, automatically (and correctly), move the references of the variables in the log entry function calls too.
            IncrementVarSublevelsUp((Node*)rewritten_subselect, 1, 1);
            sublink->subselect = (Node*)subselect_wrapper;
        }else{
            sublink->subselect = (Node*)rewritten_subselect;
        }
        return (Node*)sublink;
    }

    // Otherwise, just call the expression mutator.
    return expression_tree_mutator(node, rewrite_sublinks_mutator, (void *)context);
}
