// Common node definitions.
#ifndef TP_NODE_HPP
#define TP_NODE_HPP
#include <vector>
#include <unordered_map>
#include <string>

extern "C" {
    #include <stdint.h>
    #include "tp_list.h"
    #include "traceprov_graph.h"

    typedef std::pair<uint8_t, struct traceprov_aggregate_layer *> TraceProvWorkerLayer;
    struct TraceProvRelationArgs;

    typedef std::pair<uint64_t, uint64_t> TraceProvColumn;

    enum TraceProvNodeKind {
        T_TP_RELATION,
        T_TP_JOIN,
        T_TP_APPEND,
        T_TP_WINDOW_READ,
        T_TP_FILTER,
        T_TP_EXISTS
    };

    typedef struct TraceProvDescriptor {
        TraceProvLayerNumber layer_number;
        TraceProvEntry *entry;
    } TraceProvDescriptor;

    typedef struct {
        TraceProvDescriptor *descriptor;
        std::vector<uint64_t> *data;
        // Ugh, this makes me cry.
        // TODO: Optimize optmize optimize.
        std::vector<bool> *validity;
    } TraceProvColumnData;

    typedef std::vector<TraceProvColumnData*> TraceProvData;

    typedef struct TraceProvTopResult {
        bool is_lazy;
        List *pdata;
        uint64_t width;
        uint64_t result_count;
    } TraceProvTopResult;

    typedef std::pair<TraceProvColumn*, TraceProvColumn*> TraceProvJoinPair;
    typedef std::pair<TraceProvColumn*, uint64_t> TraceProvConstJoinPair;
    typedef std::vector<TraceProvJoinPair*> TraceProvJoinConditions;
    typedef std::vector<TraceProvConstJoinPair*> TraceProvConstJoinPairs;
    typedef std::vector<uint64_t> TraceProvOffset;

    typedef struct TraceProvNode {
        TraceProvNodeKind tag;
        // This doesn't need to be set until conversion to SQL.
        char *alias_name;
    } TraceProvNode;

    typedef struct TraceProvRelation {
        TraceProvNodeKind tag;
        char *alias_name;
        TraceProvData *data;
        char *name;
        TraceProvRelationArgs *rel_args;
    } TraceProvRelation;

    typedef struct TraceProvJoinExpr {
        TraceProvNodeKind tag;
        char *alias_name;
        TraceProvNode *left;
        TraceProvNode *right;
        // The join condition.
        TraceProvJoinConditions *join_condition;
        // The const join conditions where not supported initially.
        // This makes everything else before "just" work
        // \_ .. _/
        TraceProvConstJoinPairs *const_join_condition;
        std::vector<TraceProvColumn*> *output_columns;
        TraceProvData *result;
        bool is_left_star;
        bool is_right_star;
        bool is_single_result;
    } TraceProvJoinExpr;

    typedef struct TraceProvAppend {
        TraceProvNodeKind tag;
        char *alias_name;
        // List of TraceProvNode (get evaulated separately)
        // TraceProvAppend just appends the results individually.
        List *nodes;
        bool is_lazy;
    } TraceProvAppender;

    // Describes window log read.
    typedef struct TraceProvWindowRead {
        TraceProvNodeKind tag;
        char *alias_name;
        TraceProvRelationArgs *rel_args;
        uint64_t frame_start_idx;
        uint64_t frame_end_idx;
        // This could _technically_ be infered each time.
        // but caching ftw.
        uint64_t column_count;
        TraceProvNode *child_node;
    } TraceProvWindowRead;

    // Describes a filter on the child node.
    // Doesn't, trivially, affect the width.
    typedef struct TraceProvFilter {
        TraceProvNodeKind tag;
        char *alias_name;
        // Currently only const join conditions are supported.
        // We don't need any thing more.....yet
        TraceProvConstJoinPairs *const_join_condition;
        TraceProvNode *child_node;
    } TraceProvFilter;

    // Used for cases where there's a strict "sink"
    // For example, aggregation without any groups.
    // In such cases, we're wasting time making a join, because all
    // rows should be present.
    typedef struct TraceProvExists {
        TraceProvNodeKind tag;
        char *alias_name;
        // The remaining node
        TraceProvNode *current;
        // The node that'll be wrapped in EXISTS.
        TraceProvNode *condition;
    } TraceProvExists;

    typedef struct TraceProvDerivedNode {
        TraceProvLayerNumber layer_number;
        TraceProvNode *node;
    } TraceProvDerivedNode;

    typedef struct TraceProvEvaluateNodeContext {
        bool should_dump;
    } TraceProvEvaluateNodeContext;

    typedef struct TraceProvPointerContext{
        std::unordered_map<TraceProvLayerNumber, std::vector<uint32_t> *> *map;
        std::unordered_map<TraceProvLayerNumber, std::vector<uint8_t> *> *size_map;
    } TraceProvPointerContext;

    typedef std::pair<TraceProvJoinConditions *, TraceProvDependency *> TraceProvSublinkMapInferItem;

    // This gets used to determine whether we're done processing everything we need for a sublink.
    // It is possible that multiple paths exist to a sublink. So, that's why each graph gets a map to sublink.
    typedef std::unordered_map<TraceProvLayerNumber, uint32> TraceProvDepthMap;
    typedef std::unordered_map<uint32, std::vector<TraceProvLayerNumber> *> TraceProvSizeLayers;
    typedef std::unordered_map<TraceProvLayerNumber, TraceProvNode *> TraceProvResultMap;
    typedef struct TraceProvDerivationSpec {
        TraceProvResultMap *result_map;
        TraceProvPointerContext *p_context;
    } TraceProvDerivationSpec;

    typedef std::unordered_map<TraceProvLayerNumber, List *> TraceProvPendingSublinks;
    // Just so they can be processed together
    typedef struct TraceProvRecursePack {
        const TraceProvDepthMap *depth_map;
        const uint32 level;
        // If the level for a sublink has not been reached,
        // the pending are stored in pending sublinks.
        TraceProvPendingSublinks *pending_sublinks;
        TraceProvSizeLayers *size_layer_map;
    } TraceProvRecursePack;

    // the abstract tree for join computation.
    // Only the leaf nodes are allowed to have a valid node(s)
    // Techncally, it can be posible that we represent multiple nodes
    // as separate children but that gets unnecessarily verbose in places.
    // Everything that has the same path (starting at the root) gets "folded" into
    // union all.
    // This is a technically a graph (because the layer number can get repeated)
    typedef struct TraceProvInferAbstractTree {
        TraceProvLayerNumber layer_number;
        std::vector<TraceProvInferAbstractTree *> *children;
        std::vector<TraceProvNode *> *nodes;
    } TraceProvInferAbstractTree;

    typedef struct TraceProvRelationArgs {
        uint64_t worker_id;
        uint64_t layer_number;
        uint64_t table_flags;
        // -1 if everything. Otherwise >= 0.
        int64_t offset;
    } TraceProvRelationArgs;

    typedef struct TraceProvToSQLContext {
        TraceProvParseContext *context;
        bool use_table_def;
        TraceProvSizeLayers *size_layer_map;
        // If set, it'll record the base relations too.
        std::vector<std::string> *ddls;
        std::vector<std::pair<uint64_t, uint64_t>> *added_ddls;
        // So that, in relation scans, we can wrap this 
        TraceProvPointerContext *pointer_context;
    } TraceProvToSQLContext;

    typedef struct TraceProvInferSetupExtra {
        uint32 worker_count;
        TraceProvSizeLayers *size_layer_map;
    } TraceProvInferSetupExtra;

    typedef struct TraceProvInferResult {
        uint32_t width;
        uint64_t time;
        uint64_t row_count;
    } TraceProvInferResult;

    typedef struct TraceProvDerivation {
        List *derived_join_exprns;
        List *utilized_sublinks;
    } TraceProvDerivation;
}

#endif