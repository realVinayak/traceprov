#include "traceprov_parse_context.h"
#include "postgres.h"
#include "lib/stringinfo.h"
#include "traceprov.h"
#include "miscadmin.h"

// to simulate classes.
TraceProvLayerNumber tpParseGetLayerNumber(TraceProvParseContext *context){
    TraceProvLayerNumber current = context->global_layer_number;
    context->global_layer_number += TRACEPROV_LAYER_INCREMENT_BOUNDARY;
    return current;
}

void tpParseInitializeContext(TraceProvParseContext *context){
    // The first layer is 1.
    context->global_layer_number = 1;
    context->unique_idx = 0;
    context->simple_incrementor = 0;
}

char *tpParseGetUniqueAlias(TraceProvParseContext *context){
    unsigned long long int incremented = context->unique_idx++;
    return psprintf("tp_table_%lld", incremented);
}

int tpParseGetUniqueNumber(TraceProvParseContext *context){
    return ++(context->simple_incrementor);
}

TraceProvEntry *makeTraceProvEntry(){
    return palloc0_object(TraceProvEntry);
}

TraceProvDependency *makeTraceProvDependency(
    TraceProvLayerNumber headNumber,
    List *children,
    List *entries
){
    TraceProvDependency *graphNode = palloc0_object(TraceProvDependency);
    graphNode->headNumber = headNumber;
    graphNode->children = children;
    graphNode->entries = entries;
    return graphNode;
}

static char *serializeTraceProvEntry(TraceProvEntry *entry){
    StringInfoData buf;
    initStringInfo(&buf);
    char *kindStr = "TP_ENTRY_INVALID";
    if (entry->kind == TP_ENTRY_KIND_BASE_RELATION){
        kindStr = "TP_ENTRY_KIND_BASE_RELATION";
    }else if (entry->kind == TP_ENTRY_KIND_POINTER){
        kindStr = "TP_ENTRY_KIND_POINTER";
    }
    appendStringInfo(
        &buf, 
        "[TraceProvEntry (kind: %s, relid: %d, attr: %d)]",
        kindStr,
        entry->relId,
        entry->attr
    );
    return buf.data;
}

void appendIndentAware(StringInfoData *buf, int indentCount){
    for (int i = 0; i < indentCount; i++){
        appendStringInfo(buf, "\t");
    }
}

char * traceprovDependencyToString(int indent, const TraceProvDependency *graph){
    StringInfoData buf;
    initStringInfo(&buf);
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "TraceProv Dependency Graph {\n");
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "\theadNumber: %d\n", graph->headNumber);
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "\tentries: [");

    ListCell *entryCursor;
    foreach(entryCursor, graph->entries){
        TraceProvEntry *entry = (TraceProvEntry*)lfirst(entryCursor);
        appendStringInfo(&buf, "%s", serializeTraceProvEntry(entry));
    }
    appendStringInfo(&buf, "]\n");
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "\tchildren: [");
    ListCell *childCursor;
    foreach(childCursor, graph->children){
        appendStringInfo(&buf, "\n");
        TraceProvDependency *child = (TraceProvDependency *)lfirst(childCursor);
        appendStringInfo(&buf, "\t%s", traceprovDependencyToString(indent + 1, child));
    }
    if (list_length(graph->children)) appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "\t]\n");
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "}\n");
    return buf.data;
}

void traceprovPrintDependency(const TraceProvDependency *graph){
    elog(INFO, "TraceProvDependency: ");
    elog(INFO, "\n%s", traceprovDependencyToString(0, graph));
}

// void serializeTraceProvDependencies(List *graphs);

// This is not in the header for a reason, nothing outside of this file
// should know that this even exists.
typedef struct TraceProvDependencyHeader {
    uint32 idx; // own's index (each block has a unique index)
    uint32 numberOfEntries; // Number of entries
    uint32 numberOfDirectChildren; // Number of direct children.
} TraceProvDependencyHeader;

void failSafeWrite(FILE *file, const void *buff, size_t length){
    size_t written = fwrite(buff, length, 1, file);
    if (written != 1){
        elog(ERROR, "Error dumping the graph!");
    }
}

void failSafeRead(FILE *file, void *buff, size_t length){
    const size_t readValues = fread(buff, length, 1, file);
    if (readValues != 1){
        elog(ERROR, "Error reading from the graph file!");
    }
}

void _serializeTraceProvDepedency(const TraceProvDependency *, FILE *);
const TraceProvDependency *_deserializeTraceProvDependency(FILE *);

void serializeTraceProvDepedency(const TraceProvDependency *graph){
    const char *dumpPath = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dumpPath, "wb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    elog(INFO, "Serializing: ");
    traceprovPrintDependency(graph);
    _serializeTraceProvDepedency(graph, fptr);
    fclose(fptr);
}

void _serializeTraceProvDepedency(const TraceProvDependency *graph, FILE *outputFile){
    TraceProvDependencyHeader *header = palloc0_object(TraceProvDependencyHeader);
    header->numberOfEntries = list_length(graph->entries);
    header->numberOfDirectChildren = list_length(graph->children);
    failSafeWrite(outputFile, header, sizeof(TraceProvDependencyHeader));
    failSafeWrite(outputFile, &graph->headNumber, sizeof(graph->headNumber));
    
    ListCell *entryCursor;
    // Write the entries next.
    foreach(entryCursor, graph->entries){
        const TraceProvEntry *entry = (TraceProvEntry *)lfirst(entryCursor);
        failSafeWrite(outputFile, entry, sizeof(TraceProvEntry));
    }

    ListCell *childCursor;
    foreach(childCursor, graph->children){
        const TraceProvDependency *childGraph = (TraceProvDependency *)lfirst(childCursor);
        _serializeTraceProvDepedency(childGraph, outputFile);
    }
}

const TraceProvDependency* deserializeTraceProvDependency(){
    const char *dumpPath = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dumpPath, "rb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    const TraceProvDependency*graph = _deserializeTraceProvDependency(fptr);
    fclose(fptr);
    elog(INFO, "Deserializing: ");
    traceprovPrintDependency(graph);
    return graph;
}

const TraceProvDependency *_deserializeTraceProvDependency(FILE *file){
    TraceProvDependencyHeader header;
    failSafeRead(file, &header, sizeof(TraceProvDependencyHeader));
    TraceProvDependency *graph = palloc0_object(TraceProvDependency);
    failSafeRead(file, &graph->headNumber, sizeof(graph->headNumber));

    List *entries = NIL;
    for (uint32 idx = 0; idx < header.numberOfEntries; idx++){
        TraceProvEntry *entry = palloc0_object(TraceProvEntry);
        // Here, we should never get an eof.
        failSafeRead(file, entry, sizeof(TraceProvEntry));
        entries = lappend(entries, entry);
    }

    List *children = NIL;
    for (uint32 idx = 0; idx < header.numberOfDirectChildren; idx++){
        children = lappend(children, (void*) _deserializeTraceProvDependency(file));
    }
    
    graph->children = children;
    graph->entries = entries;
    return graph;
}