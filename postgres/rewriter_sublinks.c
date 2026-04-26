#include "rewriter_sublinks.h"
#include "nodes/pathnodes.h"
#include "rewrite/rewriteManip.h"
#include "catalog/pg_type_d.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "parser/parse_coerce.h"

static Node *rewrite_sublinks_mutator(Node *, TraceProvParseContext *);
static Node* generate_pushdown_qual(SubLink *candidate_sublink,TraceProvSublinkContext *sublink_context);
static CaseExpr *make_case_exprn(Expr *quals, Expr *result, Expr *def_result);
static void split_test_expr(Node *test_expr, Node ***left_node, Node ***right_node);

TraceProvParseContext *toggle_in_filter(TraceProvParseContext *context, Node *node){
    TraceProvParseContext *toggled = traceprov_shallow_copy_context(context);
    toggled->sub_context.is_in_filter = true;
    // The qual copy is used to determine the case when expression.
    toggled->sub_context.qual_copy = copyObject(node);
    // Not setting this is a micro optimization (will be 0 from shallow copy)
    // toggled->sub_context.seen_or = false;
    return toggled;
}

// To a given sublink, there is a stack of provenance attributes are available
// for logging. This needs to be a stack, since inner levels can (potentially) refer to
// attributes arbitrarily up.
// The rewrite is implemented using walkers (over the expressions)
void traceprov_rewrite_sublinks(Query *query, TraceProvParseContext *context, const List *added_targets_per_rte, const bool process_later){
    if (!query->hasSubLinks) return;

    context->parent_targets = lappend(context->parent_targets, (List*)added_targets_per_rte);

    // Some types of sublink entries need to be processed later (like having clause)
    if (!process_later){

        query->targetList = (List*)rewrite_sublinks_mutator((Node*)query->targetList, context);
        query->jointree->fromlist = (List *)rewrite_sublinks_mutator((Node *)query->jointree->fromlist, context);
        // The next shallow copy doesn't hurt us anyways.
        query->jointree->quals = (Node *)rewrite_sublinks_mutator((Node *)query->jointree->quals, toggle_in_filter(context, query->jointree->quals));
    }else{
        query->havingQual = rewrite_sublinks_mutator((Node*)query->havingQual, toggle_in_filter(context, query->havingQual));
    }
}

static Node *rewrite_sublinks_mutator(Node *node, TraceProvParseContext *context){
    // base case.
    if (node == NULL) return NULL;

    if (IsA(node, BoolExpr)){
        BoolExpr *bool_expr = (BoolExpr*)node;
        // Don't do anything it is a not.
        if (bool_expr->boolop == NOT_EXPR) return node;
        context->sub_context.has_seen_or |= bool_expr->boolop == OR_EXPR;
    }

    if (IsA(node, SubLink)){
        SubLink *original_sublink = (SubLink *)node;
        SubLink *sublink = copyObject(original_sublink);
        TraceProvParseContext *new_context = traceprov_shallow_copy_context(context);
        List *added_targets = NIL;
        if (!IsA(sublink->subselect, Query)){
            elog(ERROR, "Expected subselect to be a query node");
        }
        Query *subselect_query = (Query *)sublink->subselect;
        const int original_target_count = list_length(subselect_query->targetList);
        Query *rewritten_subselect = traceprov_perform_rewrite(
            subselect_query, 
            &added_targets,
            new_context,
            true
        );
        if ((original_target_count + list_length(added_targets)) != (list_length(rewritten_subselect->targetList))){
            elog(ERROR, "Unexpected target list addition!");
        }
        // Figure out the correlated references, and also log the provenance attributes of them.
        // This is done by going through the queue of provenance attributes, level by level.
        List *correlated_arg_vars = NIL;
        ListCell *provenance_queue = NULL;
        List *correlated_provenance_targets = NIL;
        // These are the cases where we'll need a qual push down to the case when exprn.
        // This is done by copying the expression and modifying it a bit.
        const bool will_need_qual_pushdown = (
            context->sub_context.is_in_filter && // Needs to be in a filter (if not, we don't care.)
            context->sub_context.has_seen_or // And we've seen an OR above us.
        );

        const bool is_any = sublink->subLinkType == ANY_SUBLINK;

        TraceProvLayerNumber layer_number = tp_parse_get_layer_number(context);

        foreach(provenance_queue, traceprov_reverse_list(context->parent_targets)){
            const int var_level_id = foreach_current_index(provenance_queue) + 1;
            const List *provenance_targets_per_rte = (List*)lfirst(provenance_queue);
            List *correlated_targets = pull_vars_of_level_ignore_sublinks((Node*)rewritten_subselect, var_level_id);

            // In some cases, we're only correlating with a specific table (and not all of them)
            // Need to remember which cases are those. Ugh, this is a bit messy and hacky. Clean up.
            uint32_t correlated_table_id = 0;
            uint32_t correlated_target_exprn_id = 0;
            uint32_t distant_correlated_id = 0;

            if ( is_any && (var_level_id == 1) && (!will_need_qual_pushdown)){
                // If the query is an ANY sublink, we can do some more optimization.
                // Of course, if is in a filter and have seen any OR, we cannot do anything.
                // Otherwise, we can add the targets that are being used as part of the ANY expression as provenance attributes.
                // This avoids having to deal with correlated expressions, in "safe" cases (all ANDs, without original correlation)
                // Effectively, we are "pushing" the IN to the backtrace stage.
                Node **left_node = NULL, **right_node = NULL;
                split_test_expr(sublink->testexpr, &left_node, &right_node);
                if (!IsA(*left_node,Var)){
                    elog(ERROR, "Expected the left node to always be a var, for now.");
                }

                // We also expect the left node to be a relation var.
                const Var *left_node_var = (Var *)*left_node;
                // Check if any of the exprn of the traceprov targets is this.
                List **p_candidate_provenance_targets = (List **)(list_nth(provenance_targets_per_rte, left_node_var->varno - 1));
                List *candidate_provenance_targets = *p_candidate_provenance_targets;
                ListCell *cand_prov_target_cursor;
                foreach(cand_prov_target_cursor, candidate_provenance_targets){
                    const TraceProvTarget *prov_target = ((TraceProvTarget *)lfirst(cand_prov_target_cursor));
                    // If it is not a Var, we cannot check against the traceprov target.
                    if (!(IsA(prov_target->targetEntry->expr, Var))) continue;
                    const Var *prov_target_var = (Var *)prov_target->targetEntry->expr;
                    if (prov_target_var->varattno == left_node_var->varattno && prov_target_var->varno == left_node_var->varno){
                        correlated_table_id = prov_target_var->varno;
                        correlated_target_exprn_id = prov_target_var->varattno;
                        break;
                    }
                }
                if (correlated_table_id == 0 && correlated_target_exprn_id == 0){
                    // In this case, we found that we're unable to find a TraceProv target on the left side.
                    // Whatever the Var that we're comparing against does not belong in the traceprov targets.
                    // So, need to add them to the corresoponding Var.
                    // However, it should only be done if the input is castable to a bigint (because that's all we log)
                    if (left_node_var->vartype != INT4OID && left_node_var->vartype != INT8OID){
                        // TODO: Check for castable here, rather than this crude check.
                        elog(ERROR, "We do not support this optimization for cases where input is not int4 or int8!");
                    }
                    Var *prov_target_var = copyObject(left_node_var);
                    TargetEntry *prov_te = makeTargetEntry(prov_target_var, 0, psprintf("in_target_%d", tp_parse_get_unique_number(context)), false);
                    TraceProvTarget *prov_target = makeTraceProvTarget(
                        false,
                        prov_te,
                        NULL,
                        0,
                        false,
                        NIL,
                        NULL,
                        false,
                        false
                    );
                    traceprov_target_set_nullable(prov_target);
                    prov_target->is_in_correlation = true;
                    *p_candidate_provenance_targets = lappend(*p_candidate_provenance_targets, prov_target);
                    correlated_table_id = left_node_var->varno;
                    correlated_target_exprn_id = left_node_var->varattno;
                }

                // Doesn't matter if we're dealing with an "invalid" var, since we only look at the varno and var levels up field.
                correlated_targets = lappend(correlated_targets, makeVar(correlated_table_id, 1, InvalidOid, -1, InvalidOid, var_level_id));

                // The distant correlated argument could be something other than the first one.
                // Need to figure out what that correlation will be.
                if (!IsA(*right_node, Param)){
                    elog(ERROR, "Expected the right side to be a param!");
                }
                TargetEntry *distant_te = list_nth(rewritten_subselect->targetList, ((Param *)(*right_node))->paramid - 1);
                if (distant_te->resorigtbl == 0 || distant_te->resorigtbl == 0){
                    elog(ERROR, "Expected them to be set, for now");
                }
                ListCell *distant_tp_target_cursor;
                foreach(distant_tp_target_cursor, added_targets){
                    const TraceProvTarget *tp_target =  (TraceProvTarget *)lfirst(distant_tp_target_cursor);
                    const TargetEntry *distant_tp_te = tp_target->targetEntry;
                    if (distant_te->resorigcol  == distant_tp_te->resorigcol && distant_te->resorigtbl == distant_tp_te->resorigtbl){
                        distant_correlated_id = foreach_current_index(distant_tp_target_cursor) + 1;
                    }
                }
                if (distant_correlated_id == 0){
                    // Need to also insert into added targets the columns with which we just correlated (didn't find these in the traceprov target)
                    TraceProvTarget *prov_target = makeTraceProvTarget(
                        false,
                        distant_te,
                        NULL,
                        0,
                        false,
                        NIL,
                        NULL,
                        false,
                        false
                    );
                    prov_target->is_in_correlation = true;
                    traceprov_target_set_nullable(prov_target);
                    added_targets = list_insert_nth(added_targets, 0, prov_target);
                    distant_correlated_id = ((Param *)(*right_node))->paramid;
                }

                if (distant_correlated_id == 0){
                    elog(ERROR, "Expected distant correlated id to be set now!");
                }

            }

            // no correlated vars case.
            const bool has_no_correlations = list_length(correlated_targets) == 0;

            // This conveniently covers good chunk of TPC-H queries :)
            if (!will_need_qual_pushdown && // Must not need qual push down
                !((Query *)original_sublink->subselect)->hasSubLinks && // Must not have any of its own sublinks
                has_no_correlations && // Must not have any correlations
                (list_length((context->parent_targets)) == 1) // Should be the depth-1 sublinks.
            ){
                continue;
            }

            ListCell *correlated_var_cursor;

            Bitmapset *added_correlated_varnos = NULL;
            // Only add the extra provenance attributes for the top level.
            if (has_no_correlations && var_level_id == 1){
                // In this case, we want to inject correlation, but don't have any.
                // Synthetically create 1.
                correlated_targets = list_make1(makeVar(1, 1, InvalidOid, -1, InvalidOid, var_level_id));
            }

            foreach(correlated_var_cursor, correlated_targets){
                const Var *correlated_var = (Var *)lfirst(correlated_var_cursor);
                if (correlated_table_id != 0 && correlated_table_id != correlated_var->varno) continue;
                const bool is_fake_correlation = correlated_table_id != 0;
                if (bms_is_member(correlated_var->varno, added_correlated_varnos)) continue;
                // If tiis table was present before, don't bother addinng it again.. 
                added_correlated_varnos = bms_add_member(added_correlated_varnos, correlated_var->varno);
                List *provenance_targets = *(List **)list_nth(provenance_targets_per_rte, correlated_var->varno - 1);
                if (correlated_target_exprn_id != 0 && is_fake_correlation){
                    TraceProvTarget *fake_correlation_target = (TraceProvTarget *)list_nth(provenance_targets, correlated_target_exprn_id - 1);
                    provenance_targets = NIL; // Pretend that we didn't just see it
                    fake_correlation_target->sublinks = lappend(
                        fake_correlation_target->sublinks,
                        makeTraceProvTargetSublinkItem(layer_number, distant_correlated_id == 0 ? 0 : distant_correlated_id - 1)
                    );
                }
                ListCell *provenance_target_to_add;
                // Don't include provenance targets used in correlation.
                const List *filtered_provenance_targets = traceprov_filter_in_correlation(provenance_targets);
                correlated_provenance_targets = list_concat(correlated_provenance_targets, filtered_provenance_targets);
                foreach(provenance_target_to_add, filtered_provenance_targets){
                    Var *outer_var = NULL;
                    const TraceProvTarget *prov_target = ((TraceProvTarget *)lfirst(provenance_target_to_add));
                    // At this point, regardless of how it is structured, it will always be a var.
                    // Only case where we have constants are still going to be seen as vars at this level.
                    traceprov_assert_all_vars(list_make1(prov_target->targetEntry));
                    outer_var = (Var*)copyObject(prov_target->targetEntry->expr);
                    outer_var->varlevelsup = correlated_var->varlevelsup;
                    if (is_fake_correlation) continue;
                    correlated_arg_vars = lappend(
                        correlated_arg_vars, 
                        outer_var
                    );
                }
            }
        }
        if ((list_length(correlated_provenance_targets) != list_length(correlated_arg_vars))){
            elog(INFO, "Expected both correlated targets to have the same length!");
        }
        // Log the traceprov references.
        // Add the correlated targets (as keys), and the targets of the base query to the context.
        tp_add_sublink_map_item(context, correlated_provenance_targets, added_targets, layer_number);
        uint64 null_map = get_null_targets_map(correlated_provenance_targets);
        Node * layer_number_const = (Node *) makeConst(
            INT4OID, 
            -1, 
            InvalidOid,
            sizeof(int32),
            Int32GetDatum(layer_number), 
            false,
            true
        );
        List *arg_vars = list_make2(layer_number_const, makeInt8Const(null_map));
        arg_vars = list_concat(arg_vars, correlated_arg_vars);
        ListCell *target_entry_cursor;
        foreach(target_entry_cursor, added_targets){
            TraceProvTarget *tp_target = (TraceProvTarget*)lfirst(target_entry_cursor);
            TargetEntry *base_target = tp_target->targetEntry;
            // Add all the expressions. This function call will replace the current target list.
            // This could be done, potentially, better by having yet another subquery block.
            arg_vars = lappend(arg_vars, base_target->expr);
        }

        // Should be exists, and the current qual expr shouldn't contain any volatile function (because they will get reapplied...)
        // Note: if no quals, no volatile functions are found (which is good.)
        Node *current_quals = rewritten_subselect->jointree->quals;
        const bool should_use_case_when = sublink->subLinkType == EXISTS_SUBLINK && !contain_volatile_functions_after_planning((Expr*)current_quals);
        Node *extra_quals = NULL;
        if (will_need_qual_pushdown){
            // Need to, ughhhh, derive the proper qual to use.
            // This is, essentially, just copying all the references, and pushing stuff a level down, easy peasy.
            extra_quals = generate_pushdown_qual(original_sublink, &context->sub_context);
        }
        if (!should_use_case_when){
            // Replace all the extra added traceprov targets with this newer func node.
            target_entry_cursor = NULL;
            List *original_without_targets = NIL;
            foreach(target_entry_cursor, rewritten_subselect->targetList){
                ListCell *inner_cell = NULL;
                bool found = false;
                foreach(inner_cell, added_targets){
                    TraceProvTarget *inner_tp_target = ((TraceProvTarget *)lfirst(inner_cell));
                    // Don't include the inner tp target if it belongs to correlation.
                    if ((inner_tp_target->targetEntry == (TargetEntry *)lfirst(target_entry_cursor)) && (!inner_tp_target->is_in_correlation) ){
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

            if (extra_quals == NULL){
                Node *traceprov_log_fcnode = traceprov_get_function_call_node(TRACEPROV_LOG_VOLATILE_FUNC_NAME, arg_vars, NULL);
                // Now, append the newly created function call node, to the target list.
                TargetEntry *log_target_entry = makeTargetEntry((Expr*)traceprov_log_fcnode, 0, tp_parse_get_unique_alias(context), false);
                rewritten_subselect->targetList = traceprov_append_at_resjunk(original_without_targets, log_target_entry);
            }

            Query *subselect_wrapper = traceprov_make_nested_query(rewritten_subselect, context, false, false);
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

            if (extra_quals != NULL){
                // Need to wrap the query, yet again :/
                List *cased_arg_vars = list_make2(layer_number_const, makeInt8Const(null_map));
                cased_arg_vars = list_concat(cased_arg_vars, correlated_arg_vars);
                ListCell *target_entry_cursor;
                foreach(target_entry_cursor, added_targets){
                    TraceProvTarget *tp_target = (TraceProvTarget*)lfirst(target_entry_cursor);
                    TargetEntry *base_target = tp_target->targetEntry;
                    cased_arg_vars = lappend(cased_arg_vars, makeVarFromTargetEntry(1, base_target));
                }
                Node *traceprov_log_fcnode = traceprov_get_function_call_node(TRACEPROV_LOG_VOLATILE_FUNC_NAME, cased_arg_vars, NULL);
                CaseExpr *case_exprn = make_case_exprn((Expr *)extra_quals, (Expr *)traceprov_log_fcnode,  (Expr *)makeBoolConst(false, false));
                subselect_wrapper->targetList = traceprov_append_at_resjunk(
                    subselect_wrapper->targetList,
                    makeTargetEntry((Expr *)case_exprn, 0, pstrdup("case_exprn"), false)
                );
                Query *cased_subselect_wrapper = traceprov_make_nested_query(subselect_wrapper, context, true, false);
                cased_subselect_wrapper->targetList = list_delete_last(cased_subselect_wrapper->targetList);
                subselect_wrapper = cased_subselect_wrapper;
            }
            sublink->subselect = (Node*)subselect_wrapper;
        }else{
            // This is stable branch (exists.)
            Node *traceprov_log_fcnode = traceprov_get_function_call_node(TRACEPROV_LOG_FUNC_NAME, arg_vars, NULL);
            Node *new_quals = traceprov_log_fcnode;
            if (current_quals != NULL){
                Node *copied_quals = copyObject(current_quals);
                CaseExpr *case_expr = make_case_exprn((Expr *)copied_quals, (Expr*)traceprov_log_fcnode, (Expr *)makeBoolConst(false, false));
                new_quals = (Node*)makeBoolExpr(AND_EXPR, list_make2(current_quals, case_expr), -1);
            }
            rewritten_subselect->jointree->quals = new_quals;
            sublink->subselect = (Node*)rewritten_subselect;
        }
        return (Node*)sublink;
    }

    // Otherwise, just call the expression mutator.
    return expression_tree_mutator(node, rewrite_sublinks_mutator, (void *)context);
}

typedef struct SublinkFinderContext {
    SubLink *sublink;
} SublinkFinderContext;

static bool sublink_finder_walker(Node *node, SublinkFinderContext *context){
    if (node == NULL){
        return false;
    }
    if (IsA(node, SubLink)){
        SubLink *candidate = (SubLink *)node;
        // We don't recurse into sublinks, so this is fine anyways.
        return equal(candidate, context->sublink);
    }
    return expression_tree_walker(node, sublink_finder_walker, (void *)context);
}

typedef struct PushdownQualContext {
    List *current;
    SubLink *sublink;
    OpExpr *parent_op_exprn;
} PushdownQualContext;

static void get_pushdown_qual(Node *node, PushdownQualContext *context){
    if (node == NULL) return;
    SublinkFinderContext sf_context = {
        .sublink = context->sublink
    };
    if (IsA(node, BoolExpr)){
        const BoolExpr* bool_node = (BoolExpr *)node;
        if (list_length(bool_node->args) == 0) return;
        ListCell *cursor;
        Node *sublink_node = NULL;
        foreach(cursor, bool_node->args){
            // Check if there is the candidate sublink exists under the current one.
            Node *bool_node_element = lfirst(cursor);
            if (sublink_node == NULL){
                if(sublink_finder_walker(bool_node_element, &sf_context)){
                    sublink_node = bool_node_element;
                }
            }
            if (bool_node_element != sublink_node){
                // Append the expression to the bool exprn stack.
                Node *exprn = copyObject(bool_node_element);
                if (bool_node->boolop == OR_EXPR){
                    // If it is an OR, make an NOT.
                    exprn = (Node *)make_notclause((Expr *)exprn);
                }
                context->current = lappend(context->current, exprn);
            }
        }
        if (sublink_node == NULL){
            elog(ERROR, "Expected to always find the sublink in one of the bool args!");
        }
        get_pushdown_qual(sublink_node, context);
        return;
    }

    if (IsA(node, OpExpr)){
        OpExpr *op_expr = (OpExpr *)node;
        // Don't need to do anything in this case either.
        if (list_length(op_expr->args) != 2) {
            elog(ERROR, "Expected at the root to find ones with two args");
        };
        context->parent_op_exprn = op_expr;
    }
}

TargetEntry *find_target_entry_by_reso(List *target_list, int resno){
    ListCell *cursor;
    foreach(cursor, target_list){
        TargetEntry *te = lfirst_node(TargetEntry, cursor);
        if (te->resno == resno){
            return te;
        }
    }
    elog(ERROR, "Expected to always find the target expr with resno: %d", resno);
    return NULL;
}


static void split_test_expr(Node *test_expr, Node ***left_node, Node ***right_node){
    if (!IsA(test_expr, OpExpr)){
        elog(ERROR, "Expected the test exprn to be an OpExpr for ANY/ALL!");
    }
    ListCell *test_op_exprn = ((OpExpr *)test_expr)->args->elements;
    *left_node = (Node **)&test_op_exprn[0].ptr_value;
    *right_node = (Node **)&test_op_exprn[1].ptr_value;
    if (!IsA(**right_node, Param)){
        elog(ERROR, "Expected the right node to be a param!");
    }
}

static Node* generate_pushdown_qual(SubLink *candidate_sublink, TraceProvSublinkContext *sublink_context){
    Node *top_exprn = NULL;
    List *pushdown_quals = NIL;
    Node **left_node = NULL, **right_node = NULL;
    if (sublink_context->qual_copy && sublink_context->has_seen_or){
        // If there's a valid qual copy, it means we're in a where / having clause.
        // The top should always be a boolean branch for now.
        Node *qual = sublink_context->qual_copy;
        if (!IsA(qual, BoolExpr)){
            elog(ERROR, "Expected the qual to be a boolean expr, for now.");
        }
        PushdownQualContext pq_context = {
            .current = NIL,
            .parent_op_exprn = NULL,
            .sublink = candidate_sublink
        };
        get_pushdown_qual(qual, &pq_context);
        pushdown_quals = pq_context.current;
        top_exprn = (Node *)pq_context.parent_op_exprn;
        ListCell *op_args = ((OpExpr*)top_exprn)->args->elements;
        bool is_present = equal(op_args[0].ptr_value,candidate_sublink);
        if (!is_present){
            left_node = (Node **)&op_args[0].ptr_value;
            if(!equal(op_args[1].ptr_value, candidate_sublink)){
                elog(ERROR, "Expected to find the sublink in the op expr second element!");
            }
            right_node = (Node **)&op_args[1].ptr_value;
        }else{
            left_node = (Node **)&op_args[1].ptr_value;
            right_node = (Node **)&op_args[0].ptr_value;
        }
    } else if ((candidate_sublink->subLinkType == ANY_SUBLINK || candidate_sublink->subLinkType == ALL_SUBLINK)){
        top_exprn = candidate_sublink->testexpr;
        split_test_expr(top_exprn, &left_node, &right_node);
    }

    if (right_node != NULL && left_node != NULL){
        List *target_entries = ((Query*)candidate_sublink->subselect)->targetList;
        // Need to set the right node to be a var.
        TargetEntry *matching_te = find_target_entry_by_reso(target_entries, 1);

        Var *sublink_out_var = makeVar(
            1, // After our rewriting, it'll be the first table.
            1, // It'll be the first attribute
            exprType((Node *)matching_te->expr),
            -1,
            InvalidOid,
            0
        );
        // Make the respective "right" node point to the var.
        *right_node = (Node *)sublink_out_var;
    }

    // Increment all the levels up in all the vars.
    IncrementVarSublevelsUp((Node *)pushdown_quals, 1, 0);

    // This is handled outside of top exprn, because the right node's levels up should
    // not be incremented.
    if (*left_node) IncrementVarSublevelsUp(*left_node, 1, 0);
    
    if (top_exprn) pushdown_quals = lappend(pushdown_quals, top_exprn);

    if (list_length(pushdown_quals) == 0) return NULL;
    if (list_length(pushdown_quals) == 1) return (Node *)pushdown_quals->elements[0].ptr_value;
    
    return (Node *)make_andclause(pushdown_quals);
}

static CaseExpr *make_case_exprn(Expr *quals, Expr *result, Expr *def_result){
    CaseExpr *case_expr = makeNode(CaseExpr);
    CaseWhen *when = makeNode(CaseWhen);
    when->expr = quals;
    when->result = result;
    case_expr->casetype = BOOLOID;
    case_expr->casecollid = InvalidOid;
    case_expr->arg = NULL;
    case_expr->args = list_make1(when);
    case_expr->defresult = def_result;
    return case_expr;
}