#ifndef __TP_DERIVATION_UTILS__
#define __TP_DERIVATION_UTILS__
#include "traceprov_node.hpp"
#include <string>

extern "C" {
    TraceProvDependency *tp_get_sublink_graph(const TraceProvParseContext *parsed_context, TraceProvLayerNumber graph_number);
    List* traceprov_get_used_sublinks(List *graphs, List **p_sublink_depth_map, TraceProvParseContext *tp_context);
    List* deserializeTraceProvDependency(
        TraceProvParseContext **parsed_context,
        char **parsed_back_query,
        const char *traceprov_graph_file
    );
    TraceProvInferAbstractTree* makeTraceProvInferAbstractTree(const TraceProvLayerNumber layer_number);
    List *getTraceProvInferAbstractTreeNodes(const TraceProvInferAbstractTree* tree);
    TraceProvRecursePack traceprov_shallow_copy_recurse_pack(const TraceProvRecursePack *reference);
    TraceProvRecursePack traceprov_increment_recursion(const TraceProvRecursePack *reference);
    uint64_t traceprov_get_node_column_count(TraceProvNode *node);
    void tp_parse_initialize_context(TraceProvParseContext *context);
    void tp_add_set_pointer_property(TraceProvParseContext *context, const uint32 pointer, const uint32 ref);
    TraceProvRelationArgs *make_relation_args(uint64_t worker_id, uint64_t layer_number);
    TraceProvRelation *make_traceprov_relation(TraceProvData *data, char *name);
    TraceProvWindowRead *make_traceprov_window_read(
        const uint64_t frame_start_idx,
        const uint64_t frame_end_idx,
        const uint64_t column_count,
        TraceProvNode *child_node
    );
    TraceProvFilter *make_traceprov_filter(
        TraceProvNode *child_node
    );
    TraceProvExists *make_traceprov_exists(
        TraceProvNode *current,
        TraceProvNode *condition
    );
    TraceProvJoinExpr *make_traceprov_join_expr(
        TraceProvNode *left,
        TraceProvNode *right,
        TraceProvJoinConditions *join_condition,
        std::vector<TraceProvColumn*> *output_columns,
        bool is_left_star = false,
        bool is_right_star = false,
        bool is_single_result = false
    );
    TraceProvNode *make_traceprov_append(
        List *nodes,
        bool is_lazy
    );
    TraceProvJoinExpr *make_traceprov_join_from_rel(
        TraceProvNode *left_side, 
        TraceProvNode *right_side, 
        int output_column_side,
        uint64_t join_idx_offset
    );
    TraceProvJoinExpr *make_traceprov_simple_join(
        TraceProvData *self_logs, 
        TraceProvNode *join_side, 
        TraceProvParseContext *parse_context, 
        const char *rel_name,
        uint64_t join_side_idx = 1
    );
    TraceProvColumnData *traceprov_make_empty_column();
    char *tp_parse_get_unique_alias(TraceProvParseContext *context);
    TraceProvDependency *tp_get_graph_from_children(const TraceProvDependency *graph, const TraceProvLayerNumber graph_number);
    int traceprov_find_first_set_number_entry(const List *entries, int set_number);
    List *tp_get_set_pointer_property(TraceProvParseContext *context, const uint32 pointer);
    TraceProvDependency *tp_get_set_graph(const TraceProvParseContext *parsed_context, const int set_number);
    List *traceprov_set_at_offset_int(List *input_list, const uint32 offset, const int value);
    TraceProvRelation* traceprov_get_relation_from_join(TraceProvJoinExpr *join_exprn, bool right=true);
    void flattenTraceProvInferAbstractTree(TraceProvInferAbstractTree *tree, TraceProvResultMap *result_map, TraceProvParseContext *parse_context);
    std::string traceprov_node_to_sql(TraceProvNode *node, TraceProvToSQLContext context);
}

#endif