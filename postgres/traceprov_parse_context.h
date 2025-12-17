#ifndef __TRACEPROV_PARSE_CONTEXT__
#define __TRACEPROV_PARSE_CONTEXT__
#include "c.h"
#include "postgres.h"
#include "utils/palloc.h"
#include "nodes/pg_list.h"
#include "access/attnum.h"
#include "nodes/primnodes.h"
#include "nodes/plannodes.h"

#define TRACEPROV_LAYER_INCREMENT_BOUNDARY 1
#define TRACEPROV_TICKER "/*(traceprov)*/"
#define TRACEPROV_SET_TICKER "/*(traceprov-set)*/"

#define TRACEPROV_SET_GRAPH ((TraceProvDependency*)-1)

#define TRACEPROV_GRAPH_IS_VALID(x) (x != NULL && x != TRACEPROV_SET_GRAPH)

#define GET_ROOT_CONTEXT(context) (context->root_context)

typedef uint32 TraceProvLayerNumber;

typedef enum TraceProvEntryKind {
    // For base relations
    TP_ENTRY_KIND_BASE_RELATION = 0,
    // For pointers
    TP_ENTRY_KIND_POINTER = 1,
    // For set pointers (in case of unions, there are multiple graphs possible)
    // Need to look at this entry.
    // There is an edge case where, in the same target list, multiple targets can belong to same set,
    // and there are multiple sets present. To correctly handle that case, there's also a setnumber.
    // Since, for a given union op, we only take the first list, it is guaranteed that if it is composed of multiple unions,
    // horizontally, it'll still be unique enough for our purposes.
    TP_ENTRY_SET_POINTER = 2,
} TraceProvEntryKind;

// Specifies what kind of graph is this
typedef enum TraceProvGraphKind {
    TP_AGGREGATE = 0,
    TP_LOG = 1
} TraceProvGraphKind;

typedef struct TraceProvEntry {
    TraceProvEntryKind kind; // What kind of entry is being logged?
    Oid relId; // Postgres catalog object id. If it is invalid, it means it's an aggregate result (so, need to infer back more)
    AttrNumber resNo;
    AttrNumber attrNumber;
    // The set number that this entry corresponds to.
    int setNumber;
    List *sublinks;
} TraceProvEntry;

// Dependency stores which layers give information about the next ones.
// This is used during inference time.
// Dependency is always acyclic (since layer cannot depend on itself)
typedef struct TraceProvDependency {
    TraceProvGraphKind graph_type;
    TraceProvLayerNumber headNumber;
    List *children; // List of TraceProvDependency (so it is a graph)
    List *entries; // List of TraceProvEntry
} TraceProvDependency;

// In case of unions, the set may get padded (depending on all the other sets in the op.)
// We need to be able to infer which ones are from padding, and which ones are actually null.
// Thus, this gets stored during padding.
typedef struct TraceProvSetPaddingMapItem {
    int setNumber;
    int padding;
} TraceProvSetPaddingMapItem;

// In case of unions, each set can have a different depedency. To account for that case.
// need store the graph for each set separately. During inference, need to look at this set to determine
// the graph. Here, it'll be list of graphs, because a set could be constructed via using multiple graphs.
typedef struct TraceProvSetGraphMapItem {
    int setNumber;
    // Each set has an associated graph with it.
    TraceProvDependency *graph;
} TraceProvSetGraphMapItem;

typedef struct TraceProvAggregateProperty {
    TraceProvLayerNumber layer_number;
    // The strategy used for base writes.
    // These get set to AggStrategy, but need to distinguish -1 case, so
    // they are int.
    int initial_strategy;
    // The strategy used for combining
    // (can be different than strategy for base writes)
    int combine_strategy;
} TraceProvAggregateProperty;

// Some properties get stored directly in the context.
// In the graph file, this also gets later stored.
typedef struct TraceProvParseGraphProperties {
    // List of TraceProvSetPaddingMapItem.
    List *setPaddingMap;
    // List of TraceProvSetGraphMapItem.
    List *setGraphMap;
    // List of TraceProvParseContext (sublinks).
    List *sublinkMap;
    // Used to identify which functions are traceprov ones, during plan analysis.
    List *traceprov_funcs;
    // The strategy inferred from the plan. List of TraceProvAggregateProperty.
    List *aggregate_properties;
} TraceProvParseGraphProperties;

typedef struct TraceProvParseContext {
    TraceProvLayerNumber global_layer_number;
    unsigned long long int unique_idx;
    int simple_incrementor;
    TraceProvParseGraphProperties *properties;
    // Used in sublinks.
    List *parent_targets;
    struct TraceProvParseContext *root_context;
} TraceProvParseContext;

TraceProvParseContext *traceprov_shallow_copy_context(const TraceProvParseContext*);

void tpParseInitializeContext(TraceProvParseContext *);

TraceProvLayerNumber tp_parse_get_layer_number(TraceProvParseContext *);
char *tp_parse_get_unique_alias(TraceProvParseContext *);
int tp_parse_get_unique_number(TraceProvParseContext *);
void tp_add_set_padding_item(TraceProvParseContext *, int, int);
void tp_add_set_graph_item(TraceProvParseContext *, int, TraceProvDependency *);
void tp_add_sublink_map_item(TraceProvParseContext *, List *, const List*, int);
void tp_add_aggregate_property(const TraceProvParseContext *, const Agg *, TraceProvLayerNumber);

TraceProvEntry *makeTraceProvEntry();

TraceProvDependency *make_traceprov_dependency(TraceProvGraphKind, TraceProvLayerNumber, List *, List *);
void traceprovPrintDependency(const TraceProvDependency *, const TraceProvParseContext *);
void serializeTraceProvDepedency(List *, TraceProvParseContext *);
List *deserializeTraceProvDependency(TraceProvParseContext **);

void traceprovPrintContext(const TraceProvParseContext *);

typedef struct TraceProvTargetSublinkItem {
    int layer_number;
    int offset_in_key;
} TraceProvTargetSublinkItem;

TraceProvTargetSublinkItem *makeTraceProvTargetSublinkItem(
    int layer_number,
    int offset_in_key
);

typedef struct TraceProvTarget {
    bool isPointer;
    TargetEntry *targetEntry;
    TraceProvDependency *graph;
    // For each union set, there are going to be pointers that make up that set.
    // Each set has its own dependency graph (in an union)
    int setNumber;
    bool isSetPointer;
    // List of TraceProvTargetSublinkItem
    List *sublinks;
} TraceProvTarget;

TraceProvTarget *makeTraceProvTarget(
    bool isPointer, 
    TargetEntry *targetEntry,
    TraceProvDependency *dependency,
    int setNumber,
    bool isSetPointer,
    List *sublinks
);

TraceProvEntry *traceprov_resolve_entry(const TraceProvTarget *, List **, List**);

char *traceProvDependencyToJson(const TraceProvDependency *);
char *traceProvParseContextToJson(const TraceProvParseContext *);
#endif