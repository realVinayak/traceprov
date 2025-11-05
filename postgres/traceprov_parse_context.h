#ifndef __TRACEPROV_PARSE_CONTEXT__
#define __TRACEPROV_PARSE_CONTEXT__
#include "c.h"
#include "nodes/pg_list.h"
#include "access/attnum.h"

#define TRACEPROV_LAYER_INCREMENT_BOUNDARY 3
#define TRACEPROV_TICKER "/*(traceprov)*/"

typedef uint32 TraceProvLayerNumber;

typedef enum TraceProvEntryKind {
    TP_ENTRY_KIND_BASE_RELATION = 0,
    TP_ENTRY_KIND_POINTER = 1
} TraceProvEntryKind;

typedef struct TraceProvEntry {
    TraceProvEntryKind kind; // What kind of entry is being logged?
    Oid relId; // Postgres catalog object id. If it is invalid, it means it's an aggregate result (so, need to infer back more)
    AttrNumber attr;
} TraceProvEntry;

// Dependency stores which layers give information about the next ones.
// This is used during inference time.
// Dependency is always acyclic (since layer cannot depend on itself)
typedef struct TraceProvDependency {
    TraceProvLayerNumber headNumber;
    List *children; // List of TraceProvDependency (so it is a graph)
    List *entries; // List of TraceProvEntry
} TraceProvDependency;

typedef struct TraceProvParseContext {
    TraceProvLayerNumber global_layer_number;
    unsigned long long int unique_idx;
} TraceProvParseContext;

void tpParseInitializeContext(TraceProvParseContext *);

TraceProvLayerNumber tpParseGetLayerNumber(TraceProvParseContext *);
char *tpParseGetUniqueAlias(TraceProvParseContext *);

TraceProvEntry *makeTraceProvEntry();

TraceProvDependency *makeTraceProvDependency(TraceProvLayerNumber, List *, List *);
void traceprovPrintDependency(TraceProvDependency *);

#endif