#include "traceprov_parse_context.h"
#include "postgres.h"
#include "lib/stringinfo.h"

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
}

char *tpParseGetUniqueAlias(TraceProvParseContext *context){
    unsigned long long int incremented = context->unique_idx++;
    return psprintf("tp_table_%lld", incremented);
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

char * traceprovSerializeDependency(int indent, TraceProvDependency *graph){
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
        appendStringInfo(&buf, "\t%s", traceprovSerializeDependency(indent + 1, child));
    }
    if (list_length(graph->children)) appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "\t]\n");
    appendIndentAware(&buf, indent);
    appendStringInfo(&buf, "}\n");
    return buf.data;
}

void traceprovPrintDependency(TraceProvDependency *graph){
    elog(INFO, "TraceProvDependency: ");
    elog(INFO, "\n%s", traceprovSerializeDependency(0, graph));
}