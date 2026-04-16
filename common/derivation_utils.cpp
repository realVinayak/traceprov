#include "traceprov_node.hpp"
#include "derivation_utils.hpp"
#include <string>
#include <algorithm>

extern "C" {
    #include "tp_list.h"
    #include "traceprov_graph.h"
    #include "utils.h"
    #include "mem_alloc.h"

    std::string safe_append(std::string old, char *other);
    static char *traceprov_get_column_name_idx(char *alias, uint64_t column_idx);
    static std::string traceprov_get_column_select(char *alias, uint64_t column_count, bool add_bigint_cast=false, std::vector<uint32_t> *unnest_cols = NULL);
    static std::string traceprov_relation_to_sql(TraceProvRelation *relation, TraceProvToSQLContext context);
    static std::string traceprov_join_to_sql(TraceProvJoinExpr *join_expr, TraceProvToSQLContext context);
    std::string expand_alias(const char *alias_name, const uint64_t column_count);
    static std::string traceprov_append_to_sql(TraceProvAppend *append, TraceProvToSQLContext context);

    static List *get_used_sublinks_in_dependency(
        const TraceProvDependency *dependency,
        TraceProvDepthMap *depth_map,
        const uint32 recursion_level,
        TraceProvParseContext *tp_context
    ){
        List *used_layer_numbers = NIL;
        ListCell *cursor;
        // Perform everything on the child first.
        // We can, absolutely, do this in 1 shot.
        // But, doing it this way simplifies code.
        foreach(cursor, dependency->entries){
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            if (entry->kind == TP_ENTRY_KIND_POINTER){
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(dependency->children, foreach_current_index(cursor));
                used_layer_numbers = list_concat(used_layer_numbers, get_used_sublinks_in_dependency(child_graph, depth_map, recursion_level + 1, tp_context));
            }
        }
        foreach(cursor, dependency->entries){
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            if (entry->kind == TP_ENTRY_KIND_BASE_RELATION) {
                ListCell *sublink_cursor;
                foreach(sublink_cursor, entry->sublinks){
                    TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_cursor);
                    const TraceProvLayerNumber sublink_number = item->layer_number;
                    // This WON'T happen for at a single level. But, can happen if there are other RTEs that referred the same sublink.
                    // In that case, we don't really gain anything by bothering to process it again...
                    if(list_member_int(used_layer_numbers, sublink_number)) continue;
                    used_layer_numbers = lappend_int(used_layer_numbers, sublink_number);
                    // Derive everyything on this sublink.
                    const TraceProvDependency *sublink_graph = tp_get_sublink_graph(tp_context, sublink_number);
                    auto child_depth_map = new TraceProvDepthMap;
                    List *indirect_sublinks = get_used_sublinks_in_dependency(sublink_graph, child_depth_map, recursion_level, tp_context);
                    used_layer_numbers = list_concat(used_layer_numbers, indirect_sublinks);
                    if (depth_map->find(item->layer_number) == depth_map->end()){
                        depth_map->insert({item->layer_number, recursion_level});
                    }else{
                        uint32 old_level = depth_map->at(item->layer_number);
                        // This is the only case where we'd be "interested" in updating the level.
                        // But, this can never happen.
                        if (old_level < recursion_level)
                            EXIT_WITH_MESSAGE("found the current recursion level to be less than the old value. Should never happen!");
                    }
                    for (auto entry: *child_depth_map){
                        if (depth_map->find(entry.first) == depth_map->end()){
                            depth_map->insert({entry.first, entry.second});
                        }else{
                            EXIT_WITH_MESSAGE("Should never find the newer graphs!");
                        }
                    }
                }
            }
        }
        return used_layer_numbers;
    }


    List* traceprov_get_used_sublinks(List *graphs, List **p_sublink_depth_map, TraceProvParseContext *tp_context){
        // Sublinks can refer to other sublinks.
        // So, discover those cases too.
        List *used_sublinks = NIL;
        ListCell *graph_cursor;
        List *depth_maps = NIL;
        foreach(graph_cursor, graphs){
            auto depth_map = new TraceProvDepthMap;
            used_sublinks = list_concat(
                used_sublinks,
                get_used_sublinks_in_dependency(
                    (TraceProvDependency *)lfirst(graph_cursor),
                    depth_map,
                    0,
                    tp_context
                )
            );
            depth_maps = lappend(depth_maps, depth_map);
        }
        if (p_sublink_depth_map)
            *p_sublink_depth_map = depth_maps;
        return used_sublinks;
    }

    TraceProvInferAbstractTree* makeTraceProvInferAbstractTree(const TraceProvLayerNumber layer_number){
        TraceProvInferAbstractTree *tree = tp_alloc0_object(TraceProvInferAbstractTree);
        tree->layer_number = layer_number;
        tree->children =  new std::vector<TraceProvInferAbstractTree *>;
        tree->nodes = new std::vector<TraceProvNode *>;
        return tree;
    }

    List *getTraceProvInferAbstractTreeNodes(const TraceProvInferAbstractTree* tree){
        List *nodes = NIL;
        for (uint64_t idx = 0; idx < tree->nodes->size(); idx++){
            // This is a pointer to the node, because we'll be mutating that in-place :)
            nodes = lappend(nodes, &tree->nodes->at(idx));
        }
        for(auto child: *tree->children){
            nodes = list_concat(nodes, getTraceProvInferAbstractTreeNodes(child));
        }
        return nodes;
    }

    TraceProvRecursePack traceprov_shallow_copy_recurse_pack(const TraceProvRecursePack *reference){
        TraceProvPendingSublinks *new_sublinks = new TraceProvPendingSublinks;
        // that's why you don't try constructing TraceProvRecursePack on the fly kids!
        for (auto pair: *reference->pending_sublinks){
            new_sublinks->insert({pair.first, list_copy(pair.second)});
        }
        return TraceProvRecursePack{
            .depth_map = reference->depth_map,
            .level = reference->level,
            .pending_sublinks = new_sublinks,
            .size_layer_map = reference->size_layer_map
        };
    }

    TraceProvRecursePack traceprov_increment_recursion(const TraceProvRecursePack *reference){
        return traceprov_shallow_copy_recurse_pack(new TraceProvRecursePack{
            .depth_map = reference->depth_map,
            .level = reference->level + 1,
            .pending_sublinks = reference->pending_sublinks,
            .size_layer_map = reference->size_layer_map
        });
    }


    uint64_t traceprov_get_node_column_count(TraceProvNode *node){
        if (node->tag == T_TP_RELATION){
            TraceProvRelation *relation = (TraceProvRelation *)node;
            return relation->data->size();
        } else if (node->tag == T_TP_JOIN){
            TraceProvJoinExpr *join_exprn = (TraceProvJoinExpr *)node;
            uint64_t left_count = 0, right_count = 0, mid_count = 0;
            if (join_exprn->is_left_star){
                left_count = traceprov_get_node_column_count(join_exprn->left);
            }
            if (join_exprn->is_right_star){
                right_count = traceprov_get_node_column_count(join_exprn->right);
            }
            if (join_exprn->output_columns != nullptr){
                for (auto output_column: *join_exprn->output_columns){
                    if (
                        (output_column->first == 1 && join_exprn->is_left_star) ||
                        (output_column->first == 2 && join_exprn->is_right_star)
                    ){
                        continue;
                    }
                    if (output_column->first == 1) left_count = 0;
                    if (output_column->first == 2) right_count = 0;
                    mid_count++;
                }
            }

            return left_count + mid_count + right_count;
        } else if (node->tag == T_TP_APPEND){
            TraceProvAppend *append = (TraceProvAppend *)node;
            return traceprov_get_node_column_count((TraceProvNode *)list_nth(append->nodes, 0));
        } else if (node->tag == T_TP_WINDOW_READ){
            TraceProvWindowRead *window_read = (TraceProvWindowRead *)node;
            return window_read->column_count; 
        } else if (node->tag == T_TP_FILTER){
            TraceProvFilter *filter = (TraceProvFilter *)node;
            return traceprov_get_node_column_count(filter->child_node);
        } else if (node->tag == T_TP_EXISTS){
            TraceProvExists *exists = (TraceProvExists *)node;
            return traceprov_get_node_column_count(exists->current);
        }
        EXIT_WITH_MESSAGE("Got unexpected node!");
        return 0;
    }

    TraceProvRelationArgs *make_relation_args(uint64_t worker_id, uint64_t layer_number){
        auto rel_args = tp_alloc0_object(TraceProvRelationArgs);
        rel_args->worker_id = worker_id;
        rel_args->layer_number = layer_number;
        return rel_args;
    }

    TraceProvRelation *make_traceprov_relation(TraceProvData *data, char *name){
        if (data->size() == 0)
            EXIT_WITH_MESSAGE("Expected to have at least 1 column!");
        auto tp_rel = tp_alloc0_object(TraceProvRelation);
        tp_rel->tag = T_TP_RELATION;
        tp_rel->data = data;
        tp_rel->name = name;
        tp_rel->rel_args = NULL;
        return tp_rel;
    }

    TraceProvWindowRead *make_traceprov_window_read(
        const uint64_t frame_start_idx,
        const uint64_t frame_end_idx,
        const uint64_t column_count,
        TraceProvNode *child_node
    ){
        auto tp_window_read = tp_alloc0_object(TraceProvWindowRead);
        tp_window_read->tag = T_TP_WINDOW_READ;
        tp_window_read->frame_start_idx = frame_start_idx;
        tp_window_read->frame_end_idx = frame_end_idx;
        tp_window_read->column_count = column_count;
        tp_window_read->child_node = child_node;
        return tp_window_read;
    }

    TraceProvFilter *make_traceprov_filter(
        TraceProvNode *child_node
    ){
        auto tp_filter_node = tp_alloc0_object(TraceProvFilter);
        tp_filter_node->tag = T_TP_FILTER;
        tp_filter_node->const_join_condition = new TraceProvConstJoinPairs;
        tp_filter_node->child_node = child_node;
        return tp_filter_node;
    }

    TraceProvExists *make_traceprov_exists(
        TraceProvNode *current,
        TraceProvNode *condition
    ){
        auto tp_exists_node = tp_alloc0_object(TraceProvExists);
        tp_exists_node->tag = T_TP_EXISTS;
        tp_exists_node->current = current;
        tp_exists_node->condition = condition;
        return tp_exists_node;
    }

    static void traceprov_assert_is_in_range(const uint64_t column_count, TraceProvColumn *column){
        if (column->second > column_count)
            EXIT_WITH_MESSAGE("Table access out of range!");
    }

    TraceProvJoinExpr *make_traceprov_join_expr(
        TraceProvNode *left,
        TraceProvNode *right,
        TraceProvJoinConditions *join_condition,
        std::vector<TraceProvColumn*> *output_columns,
        bool is_left_star,
        bool is_right_star,
        bool is_single_result
    ){
        auto tp_join_exprn = tp_alloc0_object(TraceProvJoinExpr);
        tp_join_exprn->tag = T_TP_JOIN;
        tp_join_exprn->left= left;
        tp_join_exprn->right = right;
        const auto left_column_count = traceprov_get_node_column_count(left);
        const auto right_column_count = traceprov_get_node_column_count(right);
        for(auto condition: *join_condition){
            if (condition->first->first != 1 || condition->second->first != 2)
                EXIT_WITH_MESSAGE("Numberig is inconsistent!");
            traceprov_assert_is_in_range(left_column_count, condition->first);
            traceprov_assert_is_in_range(right_column_count, condition->second);
        }
        tp_join_exprn->join_condition = join_condition;
        tp_join_exprn->output_columns = output_columns;
        tp_join_exprn->result = nullptr;
        tp_join_exprn->is_left_star = is_left_star;
        tp_join_exprn->is_right_star = is_right_star;
        tp_join_exprn->is_single_result = is_single_result;
        tp_join_exprn->const_join_condition = new TraceProvConstJoinPairs;
        return tp_join_exprn;
    }

    // We may not actually end up returning an append, depending on how things went.
    // So, the return type is a node (rather than append node)
    TraceProvNode *make_traceprov_append(
        List *nodes,
        bool is_lazy
    ){
        if (list_length(nodes) == 0)
            EXIT_WITH_MESSAGE("Making append rel with no nodes!");
        
        if (list_length(nodes) == 1){
            return (TraceProvNode*)list_nth(nodes, 0);
        }

        auto tp_append = tp_alloc0_object(TraceProvAppend);
        tp_append->tag = T_TP_APPEND;

        ListCell *node_cursor;
        uint64_t column_count = 0;
        foreach(node_cursor, nodes){
            if (column_count == 0){
                column_count = traceprov_get_node_column_count((TraceProvNode*)lfirst(node_cursor));
                continue;
            }
            if (column_count != traceprov_get_node_column_count((TraceProvNode*)lfirst(node_cursor)))
                EXIT_WITH_MESSAGE("Got differing column count in append!");
        }
        tp_append->nodes = nodes;
        tp_append->is_lazy = is_lazy;
        return (TraceProvNode*)tp_append;
    }

    TraceProvJoinExpr *make_traceprov_join_from_rel(
        TraceProvNode *left_side, 
        TraceProvNode *right_side, 
        int output_column_side,
        uint64_t join_idx_offset
    ){
        
        TraceProvColumn *join_column_1 = new TraceProvColumn(1, join_idx_offset);
        TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
        auto join_conditions = new TraceProvJoinConditions;
        join_conditions->push_back(new std::pair<TraceProvColumn *, TraceProvColumn*>(join_column_1, join_column_2));
        auto output_columns = new std::vector<TraceProvColumn*>(output_column_side - 1);
        for (int i = 0; i < output_column_side - 1; i++){
            output_columns->at(i) = (new TraceProvColumn(2, i + 2));
        }
        
        return make_traceprov_join_expr(
                left_side,
                right_side,
                join_conditions,
                output_columns
            );
    }

    TraceProvJoinExpr *make_traceprov_simple_join(
        TraceProvData *self_logs, 
        TraceProvNode *join_side, 
        TraceProvParseContext *parse_context, 
        const char *rel_name,
        uint64_t join_side_idx
    ){

        if (self_logs == nullptr)
            EXIT_WITH_MESSAGE("Expected self logs to always be set!");

        auto right_side = (TraceProvNode *)make_traceprov_relation(self_logs, tp_psprintf("%s_%s", rel_name, tp_parse_get_unique_alias(parse_context)));
        
        return make_traceprov_join_from_rel(join_side, right_side, self_logs->size(), join_side_idx);
    }

    TraceProvColumnData *traceprov_make_empty_column(){
        auto col_data = tp_alloc0_object(TraceProvColumnData);
        col_data->data = new std::vector<uint64_t>;
        return col_data;
    }

    char *tp_parse_get_unique_alias(TraceProvParseContext *context){
        unsigned long long int incremented = GET_ROOT_CONTEXT(context)->unique_idx++;
        return tp_psprintf("tp_table_%d", incremented);
    }

    // Finds a graph in the children of a graph.
    // Not recursive..
    TraceProvDependency *tp_get_graph_from_children(const TraceProvDependency *graph, const TraceProvLayerNumber graph_number){
        ListCell *graph_cursor;
        foreach(graph_cursor, graph->children){
            TraceProvDependency *dependency = (TraceProvDependency *)lfirst(graph_cursor);
            if (dependency->headNumber == graph_number)
                return dependency;
        }
        EXIT_WITH_MESSAGE("Expected to always find the graph in children!");
        return NULL;
    }

    int traceprov_find_first_set_number_entry(const List *entries, int set_number){
        ListCell *entry_cursor;
        foreach(entry_cursor, entries){
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->setNumber == set_number){
                if (te->kind == TP_ENTRY_SET_POINTER){
                    return -1;
                }
                return foreach_current_index(entry_cursor);
            }
        }
        EXIT_WITH_MESSAGE("Didn't find the set entry!");
        return -1;
    }

    List *tp_get_set_pointer_property(TraceProvParseContext *context, const uint32 pointer){
        ListCell *cursor;
        foreach(cursor, GET_ROOT_CONTEXT(context)->properties->set_pointer_map){
            TraceProvSetPointerItem *item = (TraceProvSetPointerItem *)lfirst(cursor);
            if (item->set_pointer == pointer)
                return item->refs;
        }
        return NIL;
    }

    TraceProvDependency *tp_get_set_graph(const TraceProvParseContext *parsed_context, const int set_number){
        ListCell *graph_cursor;
        foreach(graph_cursor, GET_ROOT_CONTEXT(parsed_context)->properties->set_graph_map){
            TraceProvSetGraphMapItem *set_graph_map_item = (TraceProvSetGraphMapItem *)lfirst(graph_cursor);
            if (set_graph_map_item->setNumber == set_number)
                return set_graph_map_item->graph;
        }
        EXIT_WITH_MESSAGE("Expected to always find the set graph!");
        return NULL;
    }

    // Inserts at a list's offset.
    // The offset is 0-indexed.
    List *traceprov_set_at_offset_int(List *input_list, const uint32 offset, const int value){
        while (list_length(input_list) <= offset){
            input_list = lappend_int(input_list, 0);
        }
        input_list->elements[offset].int_value = value;
        return input_list;
    }

    TraceProvRelation* traceprov_get_relation_from_join(TraceProvJoinExpr *join_exprn, bool right){
        TraceProvNode *node = right ? join_exprn->right : join_exprn->left;
        if (node->tag != T_TP_RELATION)
            EXIT_WITH_MESSAGE("Expected to always be a relation!");
        TraceProvRelation *relation = (TraceProvRelation *)node;
        return relation;
    }

    void flattenTraceProvInferAbstractTree(TraceProvInferAbstractTree *tree, TraceProvResultMap *result_map, TraceProvParseContext *parse_context){
        List *copied = NIL;
        for (auto val: *tree->nodes){
            copied = lappend(copied, val);
        }

        if (list_length(copied) > 0 ){
            TraceProvNode *new_append = make_traceprov_append(copied, false);
            if ((result_map->find(tree->layer_number) == result_map->end())){
                result_map->insert({tree->layer_number, new_append});
            }else{
                TraceProvNode * old_node = result_map->at(tree->layer_number);
                if (old_node->tag == T_TP_APPEND){
                    TraceProvAppend *old_append = (TraceProvAppend *)old_node;
                    old_append->nodes = list_concat(old_append->nodes, copied);
                }else{
                    result_map->at(tree->layer_number) = make_traceprov_append(
                        list_make2(old_node, new_append),
                        false
                    );
                }
            }
        }

        for (auto child: *tree->children){
            flattenTraceProvInferAbstractTree(child, result_map, parse_context);
        }
    }
    std::string safe_append(std::string old, char *other){
        std::string new_str = old + (std::string(other));
        tp_free(other);
        return new_str;
    }

    static char *traceprov_get_column_name_idx(char *alias, uint64_t column_idx){
        if (alias == nullptr)
            return tp_psprintf("column_%d", column_idx);
        return tp_psprintf("%s.column_%d", alias, column_idx);
    }

static std::string traceprov_get_column_select(
        char *alias,
        uint64_t column_count,
        bool add_bigint_cast,
        std::vector<uint32_t> *unnest_cols
    ){
        std::string sql_repr;
        for (uint64_t column_idx = 0; column_idx < column_count; column_idx++){
            if (column_idx > 0) sql_repr += ',';
            std::string col_repr;
            col_repr = safe_append(col_repr, traceprov_get_column_name_idx(alias, column_idx));
            bool is_unnest = false;
            if (unnest_cols != NULL){
                is_unnest = std::find(
                    unnest_cols->begin(),
                    unnest_cols->end(),
                    column_idx
                ) != unnest_cols->end();
            }
            if (add_bigint_cast){
                col_repr += "::bigint";
                col_repr += " as ";
                col_repr = safe_append(col_repr, traceprov_get_column_name_idx(nullptr, column_idx));
            }
            if (is_unnest){
                col_repr = safe_append("unnest(traceprov_read_int_vector(" + col_repr  + ", 0)) as ", traceprov_get_column_name_idx(nullptr, column_idx));
            }
            sql_repr += col_repr;
        }
        return sql_repr;
    }

    static std::string traceprov_relation_to_sql(TraceProvRelation *relation, TraceProvToSQLContext context){
        std::string sql = "";
        sql += "SELECT ";
        std::vector<uint32_t> *unnest_cols = NULL;
        if (context.pointer_context != NULL){
            if (context.pointer_context->map->find(relation->rel_args->layer_number) != context.pointer_context->map->end()){
                unnest_cols = context.pointer_context->map->at(relation->rel_args->layer_number);
            }
        }
        sql += traceprov_get_column_select(relation->name, relation->data->size(), false, unnest_cols);
        if (context.use_table_def){
            sql = safe_append(sql, tp_psprintf(" FROM traceprov_read_worker_layer(%ld::bigint, %d::int, %d::int) AS %s", relation->rel_args->table_flags, relation->rel_args->worker_id, relation->rel_args->layer_number, relation->name));
        }else{
            sql = safe_append(sql, tp_psprintf(" FROM %s", relation->name));
        }
        if (context.ddls){
            bool found = false;
            auto curr_pair = std::pair<uint64_t, uint64_t>(relation->rel_args->worker_id, (relation->rel_args->layer_number << 32) | relation->rel_args->table_flags);
            for (auto old: *context.added_ddls){
                if (old == (curr_pair)){
                    found = true;
                    break;
                }
            }
            if (!found){
                context.ddls->push_back(sql);
                context.added_ddls->push_back(curr_pair);
            }
        }
        return sql;
    }

    static std::string traceprov_join_to_sql(TraceProvJoinExpr *join_expr, TraceProvToSQLContext context){
        std::string left_node_raw_sql = traceprov_node_to_sql(join_expr->left, context);
        std::string right_node_raw_sql = traceprov_node_to_sql(join_expr->right, context);
        char *left_alias = join_expr->left->alias_name;
        char *right_alias = join_expr->right->alias_name;
        std::string left_alias_expanded = expand_alias(left_alias, traceprov_get_node_column_count(join_expr->left));
        std::string right_alias_expanded = expand_alias(right_alias, traceprov_get_node_column_count(join_expr->right));

        char *left_node_sql = tp_psprintf("(%s) as %s", left_node_raw_sql.c_str(), left_alias_expanded.c_str());
        char *right_node_sql = tp_psprintf("(%s) as %s", right_node_raw_sql.c_str(), right_alias_expanded.c_str());

        std::string sql_repr;
        sql_repr += "SELECT ";
        bool did_append = false;

        // Expand all lefts.
        if (join_expr->is_left_star){
            did_append = true;
            const uint64_t left_column_count = traceprov_get_node_column_count(join_expr->left);
            sql_repr += traceprov_get_column_select(left_alias, left_column_count);
        }

        // Expand all the rights.
        if (join_expr->is_right_star){
            if (did_append) sql_repr += ", ";
            did_append = true;
            const uint64_t right_column_count = traceprov_get_node_column_count(join_expr->right);
            sql_repr += traceprov_get_column_select(right_alias, right_column_count);
        }

        if (join_expr->output_columns != nullptr){
            for (auto output_column: *join_expr->output_columns){
                if (
                    (output_column->first == 1 && join_expr->is_left_star) ||
                    (output_column->first == 2 && join_expr->is_right_star)
                ){
                    continue;
                }
                if (did_append) sql_repr += ", ";
                auto alias_name = output_column->first == 1 ? left_alias : right_alias;
                sql_repr += traceprov_get_column_name_idx(alias_name, output_column->second - 1);
                did_append = true;
            }
        }
        // Add the from claause.
        std::string join_repr;
        bool did_add_in_join = false;
        for (auto join_condition: *join_expr->join_condition){
            if (did_add_in_join)
                join_repr += " AND ";
            did_add_in_join = true;
            auto left_column = join_condition->first;
            auto right_column = join_condition->second;
            if (left_column->first != 1 || right_column->first != 2)
                EXIT_WITH_MESSAGE("Got invalid numbering!");

            join_repr = safe_append(
                join_repr,
                tp_psprintf(
                    "%s=%s", 
                    traceprov_get_column_name_idx(left_alias, left_column->second - 1),
                    traceprov_get_column_name_idx(right_alias, right_column->second - 1)
                )
            );
        }
        for (auto join_condition: *join_expr->const_join_condition){
            if (did_add_in_join)
                join_repr += " AND ";
            did_add_in_join = true;
            auto left_column = join_condition->first;
            auto right_column = join_condition->second;
            if (left_column->first != 1)
                EXIT_WITH_MESSAGE("Got invalid numbering!");

            join_repr = safe_append(
                join_repr,
                tp_psprintf(
                    "%s=%d", 
                    traceprov_get_column_name_idx(left_alias, left_column->second - 1),
                    right_column
                )
            );
        }
        sql_repr = safe_append(
            sql_repr, 
            tp_psprintf(
                " FROM %s JOIN %s ON (%s) ", left_node_sql, right_node_sql, join_repr.c_str()
            )
        );
        return sql_repr;
    }


    static std::string traceprov_append_to_sql(TraceProvAppend *append, TraceProvToSQLContext context){
        if (list_length(append->nodes) == 1){
            TraceProvNode *single_node =  (TraceProvNode*)list_nth(append->nodes, 0);
            std::string node_sql = traceprov_node_to_sql(single_node, context);
            // Don't bother generating a new alias for this specific case.
            // Just use whatever the child is.
            append->alias_name = single_node->alias_name;
            return node_sql;
        }
        append->alias_name = tp_parse_get_unique_alias(context.context);
        std::string append_repr;
        ListCell *node_cursor;
        foreach(node_cursor, append->nodes){
            if (foreach_current_index(node_cursor) > 0)
                append_repr += " UNION ALL ";
            TraceProvNode *node = (TraceProvNode *)lfirst(node_cursor);
            auto node_sql = traceprov_node_to_sql(node, context);
            append_repr = safe_append(append_repr, tp_psprintf(" (%s) ", node_sql.c_str()));
        }
        return append_repr;
    }

    std::string expand_alias(const char *alias_name, const uint64_t column_count){
        // Expands alias such that it is as table(col0, col1, col2...)
        auto select = traceprov_get_column_select(nullptr, column_count);
        auto alias_repr = std::string(tp_psprintf("%s(%s)", alias_name, select.c_str()));
        return alias_repr;
    }
}

std::string traceprov_node_to_sql(TraceProvNode *node, TraceProvToSQLContext context){
    std::string gen_sql = "";
    if (context.cache != NULL){
        // Check the cache, if the SQL is present, don't bother regenerating again.
        if (context.cache->find((uint64_t)node) != context.cache->end())
            gen_sql = context.cache->at((uint64_t)node);
    }
    if (gen_sql != ""){
        return gen_sql;
    }

    if (node->tag == T_TP_RELATION){
        node->alias_name = tp_parse_get_unique_alias(context.context);
        gen_sql = traceprov_relation_to_sql((TraceProvRelation *)node, context);
    } else if (node->tag == T_TP_JOIN){
        node->alias_name = tp_parse_get_unique_alias(context.context);
        gen_sql = traceprov_join_to_sql((TraceProvJoinExpr *)node, context);
    } else if (node->tag == T_TP_APPEND){
        gen_sql = traceprov_append_to_sql((TraceProvAppend *)node, context);
    } else {
        EXIT_WITH_MESSAGE("Found handling invalid node in toSQL");
    }
    // if (node->tag == T_TP_WINDOW_READ){
    //     node->alias_name = tp_parse_get_unique_alias(context.context);
    //     return traceprov_window_read_to_sql((TraceProvWindowRead *)node, context);
    // }
    // if (node->tag == T_TP_FILTER){
    //     node->alias_name = tp_parse_get_unique_alias(context.context);
    //     return traceprov_filter_to_sql((TraceProvFilter *)node, context);
    // }
    // if (node->tag == T_TP_EXISTS){
    //     node->alias_name = tp_parse_get_unique_alias(context.context);
    //     return traceprov_exists_to_sql((TraceProvExists *)node, context);
    // }
    // Insert the node's SQL into cache.
    if (context.cache){
        context.cache->insert({(uint64_t)node, gen_sql});
    }
    return gen_sql;
}
