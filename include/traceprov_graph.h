#ifndef TP_GRAPH_H
#define TP_GRAPH_H
#include <unistd.h>
#include <stdint.h>
#include "tp_list.h"

#define TRACEPROV_SET_GRAPH ((TraceProvDependency*)-1)

#define TRACEPROV_GRAPH_IS_VALID(x) (x != NULL && x != TRACEPROV_SET_GRAPH)

#define GET_ROOT_CONTEXT(context) (context->root_context)

typedef uint32_t uint32;

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
    TP_ENTRY_FRAME_INHERIT = 5,
    // Previously, this used to be whatever the type of the provenance attributes the correlated tables had.
    // But, this approach makes things tidier.
    TP_ENTRY_CORRELATION_ATTR = 6,
    // If we made an entry that corresponds to a IN, need to remember that.
    TP_ENTRY_IN_CORRELATION_ATTR = 7,
    // rowid attribute
    TP_ENTRY_ROWID = 8
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
    int32_t resNo;
    int32_t attrNumber;
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

typedef struct Node Node;

typedef struct TraceProvSublinkContext {
    Node *and_qual;
    // Need to remember if this qual is only for something in a filter.
    bool is_in_filter;
    // Have we seen an OR so far?
    bool has_seen_or;
    // If it is filter, it is the copy of the top level qual (OR having.)
    Node *qual_copy;
} TraceProvSublinkContext;


typedef struct TraceProvParseContext {
    TraceProvLayerNumber global_layer_number;
    unsigned long long int unique_idx;
    int simple_incrementor;
    TraceProvParseGraphProperties *properties;
    // Used in sublinks.
    List *parent_targets;
    struct TraceProvParseContext *root_context;
    // Some specialized things that are helpful to have for the sublinks.
    TraceProvSublinkContext sub_context;
} TraceProvParseContext;

typedef struct TraceProvTargetSublinkItem {
    TraceProvLayerNumber layer_number;
    int offset_in_key;
} TraceProvTargetSublinkItem;

typedef struct TraceProvDependencyMetaHeader {
    TraceProvLayerNumber max_layer_number;
    // Number of graphs being stored.
    uint32_t num_graphs;
    uint32_t num_set_padding_map_items;
    uint32_t num_set_graph_map_items;
    uint32_t num_sublink_items;
    uint32_t num_set_pointer_map;
} TraceProvDependencyMetaHeader;

// static_assert(sizeof(TraceProvDependencyMetaHeader) == 24);

// This is not in the header for a reason, nothing outside of this file
// should know that this even exists.
typedef struct TraceProvDependencyHeader {
    uint32_t idx; // own's index (each block has a unique index)
    uint32_t numberOfEntries; // Number of entries
    uint32_t numberOfDirectChildren; // Number of direct children.
    TraceProvDependency *graphPtr; // Useful to detect if the graph is null or not.
} TraceProvDependencyHeader;

typedef struct TraceProvStringHeader {
    size_t size;
} TraceProvStringHeader;


typedef struct TraceProvEntryMetaHeader {
    uint32 num_keys;
} TraceProvEntryMetaHeader;

typedef struct TraceProvLogSize {
    uint64_t page_requested_size;
    uint64_t page_used_size;
    uint64_t bytes_used_size;
} TraceProvLogSize;

// Is combine
#define TRACEPROV_TABLE_COMBINE (((uint64_t)1) << 0)
// Treat as sequential scan
#define TRACEPROV_TABLE_SEQ_SCAN (((uint64_t)1) << 1)
// Add first column as the row id.
#define TRACEPROV_TABLE_ROW_ID (((uint64_t)1) << 2)
#endif