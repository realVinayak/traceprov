#ifndef __TRACEPROV_PARSE_CONTEXT__
#define __TRACEPROV_PARSE_CONTEXT__
#include "c.h"
#include "postgres.h"
#include "utils/palloc.h"
#include "nodes/pg_list.h"
#include "access/attnum.h"
#include "nodes/primnodes.h"

#define TRACEPROV_LAYER_INCREMENT_BOUNDARY 3
#define TRACEPROV_TICKER "/*(traceprov)*/"
#define TRACEPROV_SET_TICKER "/*(traceprov-set)*/"

#define TRACEPROV_SET_GRAPH ((TraceProvDependency*)-1)

#define TRACEPROV_GRAPH_IS_VALID(x) (x != NULL && x != TRACEPROV_SET_GRAPH)

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

typedef struct TraceProvEntry {
    TraceProvEntryKind kind; // What kind of entry is being logged?
    Oid relId; // Postgres catalog object id. If it is invalid, it means it's an aggregate result (so, need to infer back more)
    AttrNumber resNo;
    AttrNumber attrNumber;
    // The set number that this entry corresponds to.
    int setNumber;
} TraceProvEntry;

// Dependency stores which layers give information about the next ones.
// This is used during inference time.
// Dependency is always acyclic (since layer cannot depend on itself)
typedef struct TraceProvDependency {
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
// need store the graph for each set separately. During inference, also need to do similar thing
typedef struct TraceProvSetGraphMapItem {
    int setNumber;
    TraceProvDependency *graph;
} TraceProvSetGraphMapItem;

// Some properties get stored directly in the context.
// In the graph file, this also gets later stored.
typedef struct TraceProvParseGraphProperties {
    List *setPaddingMap;
    List *setGraphMap;
} TraceProvParseGraphProperties;


typedef struct TraceProvParseContext {
    TraceProvLayerNumber global_layer_number;
    unsigned long long int unique_idx;
    int simple_incrementor;
    TraceProvParseGraphProperties *properties;
} TraceProvParseContext;

void tpParseInitializeContext(TraceProvParseContext *);

TraceProvLayerNumber tpParseGetLayerNumber(TraceProvParseContext *);
char *tpParseGetUniqueAlias(TraceProvParseContext *);
int tpParseGetUniqueNumber(TraceProvParseContext *);
void tpAddSetPaddingItem(TraceProvParseContext *, int, int);

TraceProvEntry *makeTraceProvEntry();

TraceProvDependency *makeTraceProvDependency(TraceProvLayerNumber, List *, List *);
void traceprovPrintDependency(const TraceProvDependency *);
void serializeTraceProvDepedency(const TraceProvDependency *);
const TraceProvDependency*deserializeTraceProvDependency(void);

typedef struct TraceProvTarget {
    bool isPointer;
    TargetEntry *targetEntry;
    TraceProvDependency *graph;
    // For each union set, there are going to be pointers that make up that set.
    // Each set has its own dependency graph (in an union)
    int setNumber;
    // If it is an union set, then 
    bool isSetPointer;
} TraceProvTarget;

TraceProvTarget *makeTraceProvTarget(
    bool, 
    TargetEntry *,
    TraceProvDependency *,
    int,
    bool
);

TraceProvEntry *tpResolveEntry(const TraceProvTarget *, List **, List**);

char *traceProvDependencyToJson(const TraceProvDependency *);
#endif