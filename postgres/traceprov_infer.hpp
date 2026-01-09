#ifndef __TP_INFER__
#define __TP_INFER__
#include <vector>
#include "traceprov_parse_context.h"
#include <unordered_map>

// There are two versions because in some (rare-ish) cases multiple keys form the primary key.
// Usually, that won't happen, so having a separate map is useful to not construct redundant single element vectors
typedef std::unordered_map<Oid, std::vector<Datum>> TraceProvSingleDerivation;
typedef std::unordered_map<Oid, std::vector<std::vector<Datum>>> TraceProvMultipleDerivation;

typedef std::pair<uint8, struct traceprov_aggregate_layer *> TraceProvWorkerLayer;

typedef std::pair<uint64, uint64> TraceProvColumn;

enum TraceProvNodeKind {
    T_TP_RELATION,
    T_TP_JOIN,
    T_TP_APPEND
};

typedef struct TraceProvDescriptor {
    TraceProvLayerNumber layer_number;
    TraceProvEntry *entry;
} TraceProvDescriptor;

typedef struct {
    TraceProvDescriptor *descriptor;
    std::vector<uint64> *data;
} TraceProvColumnData;

typedef std::vector<TraceProvColumnData*> TraceProvData;
typedef struct TraceProvTopResult {
    bool is_lazy;
    List *pdata;
    uint64 width;
    uint64 result_count;
} TraceProvTopResult;

typedef std::vector<std::pair<TraceProvColumn*, TraceProvColumn*>*> TraceProvJoinConditions;
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
} TraceProvRelation;

typedef struct TraceProvJoinExpr {
    TraceProvNodeKind tag;
    char *alias_name;
    TraceProvNode *left;
    TraceProvNode *right;
    // The join condition.
    TraceProvJoinConditions *join_condition;
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

#endif