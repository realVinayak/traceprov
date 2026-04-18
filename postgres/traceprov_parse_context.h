#ifndef __TRACEPROV_PARSE_CONTEXT__
#define __TRACEPROV_PARSE_CONTEXT__
#include "c.h"
#include "postgres.h"
#include "utils/palloc.h"
#include "nodes/pg_list.h"
#include "access/attnum.h"
#include "nodes/primnodes.h"
#include "nodes/plannodes.h"
#include "traceprov_graph.h"

#define TRACEPROV_LAYER_INCREMENT_BOUNDARY 1
#define TRACEPROV_TICKER "/*(traceprov)*/"
#define TRACEPROV_SET_TICKER "/*(traceprov-set)*/"

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
void tp_add_sublink_map_item(
    TraceProvParseContext *context,
    List *key_traceprov_targets,
    const List *ptr_traceprov_targets,
    const TraceProvLayerNumber layer_number
);
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
