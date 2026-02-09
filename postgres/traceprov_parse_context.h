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
    TP_ENTRY_FRAME_START = 3,
    TP_ENTRY_FRAME_END = 4,
    TP_ENTRY_FRAME_INHERIT = 5
} TraceProvEntryKind;

// Specifies what kind of graph is this
typedef enum TraceProvGraphKind {
    // So bugs can be caught.
    TP_INVALID = 0,
    TP_AGGREGATE,
    TP_LOG,
    // Like aggregate, but no group-by clauses.
    // Used for infer optimizations.
    TP_PURE_AGGREGATE
} TraceProvGraphKind;

typedef struct TraceProvWindowFrameEntry {
    TraceProvEntryKind kind; // Whether this is start, or end, or pointer
    TraceProvLayerNumber log_layer_number;
} TraceProvWindowFrameEntry;

typedef struct TraceProvEntry {
    TraceProvEntryKind kind; // What kind of entry is being logged?
    Oid relId; // Postgres catalog object id. If it is invalid, it means it's an aggregate result (so, need to infer back more)
    AttrNumber resNo;
    AttrNumber attrNumber;
    // The set number that this entry corresponds to.
    int setNumber;
    List *sublinks;
    TraceProvWindowFrameEntry window_entry;
    bool is_pointer_for_window;
    bool is_nullable;
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

// To prune some of the trees for the inference in UNION,
// need to store the possible values a set pointer can take.
typedef struct TraceProvSetPointerItem {
    uint32 set_pointer;
    List *refs;
} TraceProvSetPointerItem;

// Some properties get stored directly in the context.
// In the graph file, this also gets later stored.
typedef struct TraceProvParseGraphProperties {
    // List of TraceProvSetPaddingMapItem.
    List *set_padding_map;
    // List of TraceProvSetGraphMapItem.
    List *set_graph_map;
    // List of TraceProvDependency (sublinks).
    List *sublink_map;
    // Used to identify which functions are traceprov ones, during plan analysis.
    List *traceprov_funcs;
    // The strategy inferred from the plan. List of TraceProvAggregateProperty.
    List *aggregate_properties;
    // The list of TraceProvSetPointerItem.
    List *set_pointer_map;
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

typedef struct TraceProvTargetSublinkItem {
    TraceProvLayerNumber layer_number;
    int offset_in_key;
} TraceProvTargetSublinkItem;

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
    // The window entry this target refers to.
    TraceProvWindowFrameEntry *window_entry;
    // Whether this agg is for a window.
    // In that case, while we want to propagate it,
    // we don't gain anything from deriving on it.
    bool is_pointer_for_window;
    bool is_nullable;
} TraceProvTarget;

TraceProvParseContext *traceprov_shallow_copy_context(const TraceProvParseContext*);

void tp_parse_initialize_context(TraceProvParseContext *);

TraceProvLayerNumber tp_parse_get_layer_number(TraceProvParseContext *);
char *tp_parse_get_unique_alias(TraceProvParseContext *);
int tp_parse_get_unique_number(TraceProvParseContext *);
void tp_add_set_padding_item(TraceProvParseContext *, int, int);
void tp_add_set_graph_item(TraceProvParseContext *, int, TraceProvDependency *);
void tp_add_sublink_map_item(TraceProvParseContext *, List *, const List*, int);
TraceProvDependency *tp_get_sublink_graph(const TraceProvParseContext *parsed_context, TraceProvLayerNumber graph_number);
TraceProvDependency *tp_get_set_graph(const TraceProvParseContext *parsed_context, const int set_number);
TraceProvDependency *tp_get_graph_from_children(const TraceProvDependency *graph, TraceProvLayerNumber graph_number);
void tp_add_aggregate_property(const TraceProvParseContext *, const Agg *, TraceProvLayerNumber);
void tp_add_set_pointer_property(TraceProvParseContext *context, const uint32 pointer, const uint32 ref);
List *tp_get_set_pointer_property(TraceProvParseContext *context, const uint32 pointer);

TraceProvEntry *makeTraceProvEntry();

TraceProvDependency *make_traceprov_dependency(TraceProvGraphKind, TraceProvLayerNumber, List *, List *);
void traceprovPrintDependency(const TraceProvDependency *, const TraceProvParseContext *);
void serializeTraceProvDepedency(List *, TraceProvParseContext *, const char *);
List *deserializeTraceProvDependency(TraceProvParseContext **, char **);

void traceprovPrintContext(const TraceProvParseContext *);

TraceProvTargetSublinkItem *makeTraceProvTargetSublinkItem(
    TraceProvLayerNumber layer_number,
    int offset_in_key
);

TraceProvTarget *makeTraceProvTarget(
    bool isPointer, 
    TargetEntry *targetEntry,
    TraceProvDependency *dependency,
    int setNumber,
    bool isSetPointer,
    // List of TraceProvTargetSublinkItem.
    List *sublinks,
    TraceProvWindowFrameEntry *window_entry,
    bool is_pointer_for_window,
    bool is_nullable
);

// Does to reinitialize the nullable value.
// The transition from false->true->false is never possible, so calling
// it multiple times is safe.
void traceprov_target_set_nullable(TraceProvTarget * target);

TraceProvWindowFrameEntry *traceprov_make_window_frame_entry(
    TraceProvEntryKind kind,
    TraceProvLayerNumber log_layer_number
);

TraceProvEntry *traceprov_resolve_entry(TraceProvTarget *, List **, List**);

char *traceProvDependencyToJson(const TraceProvDependency *);
char *traceProvParseContextToJson(const TraceProvParseContext *);
#endif
