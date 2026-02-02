#ifndef __TP_INFER__
#define __TP_INFER__
#include <vector>
#include "traceprov_parse_context.h"
#include <unordered_map>
#include "traceprov_infer_essentials.hpp"

// There are two versions because in some (rare-ish) cases multiple keys form the primary key.
// Usually, that won't happen, so having a separate map is useful to not construct redundant single element vectors
typedef std::unordered_map<Oid, std::vector<Datum>> TraceProvSingleDerivation;
typedef std::unordered_map<Oid, std::vector<std::vector<Datum>>> TraceProvMultipleDerivation;

typedef std::pair<uint8, struct traceprov_aggregate_layer *> TraceProvWorkerLayer;

typedef std::pair<uint64, uint64> TraceProvColumn;

enum TraceProvNodeKind {
    T_TP_RELATION,
    T_TP_JOIN,
    T_TP_APPEND,
    T_TP_WINDOW_READ,
    T_TP_FILTER
};

typedef struct TraceProvDescriptor {
    TraceProvLayerNumber layer_number;
    TraceProvEntry *entry;
} TraceProvDescriptor;

typedef struct {
    TraceProvDescriptor *descriptor;
    std::vector<uint64> *data;
    // Ugh, this makes me cry.
    // TODO: Optimize optmize optimize.
    std::vector<bool> *validity;
} TraceProvColumnData;

typedef std::vector<TraceProvColumnData*> TraceProvData;

typedef struct TraceProvTopResult {
    bool is_lazy;
    List *pdata;
    uint64 width;
    uint64 result_count;
} TraceProvTopResult;

typedef std::pair<TraceProvColumn*, TraceProvColumn*> TraceProvJoinPair;
typedef std::pair<TraceProvColumn*, uint64> TraceProvConstJoinPair;
typedef std::vector<TraceProvJoinPair*> TraceProvJoinConditions;
typedef std::vector<TraceProvConstJoinPair*> TraceProvConstJoinPairs;
typedef std::vector<uint64> TraceProvOffset;

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
    uint64 frame_start_idx;
    uint64 frame_end_idx;
    // This could _technically_ be infered each time.
    // but caching ftw.
    uint64 column_count;
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

typedef struct TraceProvDerivation {
    TraceProvSingleDerivation single_derivation;
    TraceProvMultipleDerivation multiple_derivation;
    List *derived_join_exprns;
    List *utilized_sublinks;
} TraceProvDerivation;

typedef struct TraceProvDerivedNode {
    TraceProvLayerNumber layer_number;
    TraceProvNode *node;
} TraceProvDerivedNode;

typedef struct TraceProvEvaluateNodeContext {
    bool should_dump;
} TraceProvEvaluateNodeContext;

typedef std::pair<TraceProvJoinConditions *, TraceProvDependency *> TraceProvSublinkMapInferItem;

// This gets used to determine whether we're done processing everything we need for a sublink.
// It is possible that multiple paths exist to a sublink. So, that's why each graph gets a map to sublink.
typedef std::unordered_map<TraceProvLayerNumber, uint32> TraceProvDepthMap;
typedef std::unordered_map<uint32, std::vector<TraceProvLayerNumber> *> TraceProvSizeLayers;

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

typedef struct TraceProvToSQLContext {
    TraceProvParseContext *context;
    bool use_table_def;
    TraceProvSizeLayers *size_layer_map;
} TraceProvToSQLContext;

typedef struct TraceProvInferSetupExtra {
    uint32 worker_count;
    TraceProvSizeLayers *size_layer_map;
} TraceProvInferSetupExtra;


extern "C" {
    TraceProvData *traceprov_perform_duckdb_inference(const char *generated_sql, struct traceprov_inference_context *context);
    void traceprov_duckdb_setup_context(
        struct traceprov_inference_context **p_ctxt,
        void (**p_cleanup)(struct traceprov_inference_context *),
        TraceProvInferSetupExtra *setup_extra
    );
}

#endif