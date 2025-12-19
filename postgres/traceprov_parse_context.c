#include "traceprov_parse_context.h"
#include "postgres.h"
#include "lib/stringinfo.h"
#include "traceprov.h"
#include "miscadmin.h"


#define NEED_SEP(cursor) (foreach_current_index(cursor) > 0)

// to simulate classes.
TraceProvLayerNumber tp_parse_get_layer_number(TraceProvParseContext *context){
    TraceProvLayerNumber current = GET_ROOT_CONTEXT(context)->global_layer_number;
    GET_ROOT_CONTEXT(context)->global_layer_number += TRACEPROV_LAYER_INCREMENT_BOUNDARY;
    return current;
}

void tpParseInitializeContext(TraceProvParseContext *context){
    // The first layer is 1.
    context->global_layer_number = 1;
    context->unique_idx = 0;
    context->simple_incrementor = 0;
    context->properties = palloc0_object(TraceProvParseGraphProperties);
    context->properties->setPaddingMap = NIL;
    context->properties->setGraphMap = NIL;
    context->properties->sublinkMap = NIL;
    context->parent_targets = NIL;
}

char *tp_parse_get_unique_alias(TraceProvParseContext *context){
    unsigned long long int incremented = GET_ROOT_CONTEXT(context)->unique_idx++;
    return psprintf("tp_table_%lld", incremented);
}

int tp_parse_get_unique_number(TraceProvParseContext *context){
    return ++(GET_ROOT_CONTEXT(context)->simple_incrementor);
}

// Add the set number (and the performed padding) to the context's properties
void tp_add_set_padding_item(TraceProvParseContext *context, int setNumber, int padding){
    TraceProvSetPaddingMapItem *mapItem = palloc0_object(TraceProvSetPaddingMapItem);
    mapItem->setNumber = setNumber;
    mapItem->padding = padding;
    GET_ROOT_CONTEXT(context)->properties->setPaddingMap = lappend(GET_ROOT_CONTEXT(context)->properties->setPaddingMap, mapItem);
}

void tp_add_set_graph_item(TraceProvParseContext *context, int setNumber, TraceProvDependency*graph){
    TraceProvSetGraphMapItem *setGraphMapItem = palloc0_object(TraceProvSetGraphMapItem);
    setGraphMapItem->graph = graph;
    setGraphMapItem->setNumber = setNumber;
    GET_ROOT_CONTEXT(context)->properties->setGraphMap = lappend(GET_ROOT_CONTEXT(context)->properties->setGraphMap, setGraphMapItem);
}

TraceProvTargetSublinkItem *makeTraceProvTargetSublinkItem(
    int layer_number,
    int offset_in_key
){
    TraceProvTargetSublinkItem *item = palloc0_object(TraceProvTargetSublinkItem);
    item->layer_number = layer_number;
    item->offset_in_key = offset_in_key;
    return item;
}

void tp_add_sublink_map_item(
    TraceProvParseContext *context,
    // Need to also store, for each key target, what the corresponding entry is
    List *key_traceprov_targets,
    const List *ptr_traceprov_targets,
    int layer_number
){
    ListCell *target_entry_cursor;
    List *entries = NIL;
    List *child_graphs = NIL;
    foreach(target_entry_cursor, ptr_traceprov_targets){
        TraceProvEntry *tpEntry = traceprov_resolve_entry(((TraceProvTarget *)lfirst(target_entry_cursor)), &child_graphs, NULL);
        entries = lappend(entries, tpEntry);
    }
    const TraceProvDependency *graph = make_traceprov_dependency(
        TP_LOG,
        layer_number,
        child_graphs,
        entries
    );
    GET_ROOT_CONTEXT(context)->properties->sublinkMap = lappend(GET_ROOT_CONTEXT(context)->properties->sublinkMap, (TraceProvDependency*)graph);
    target_entry_cursor = NULL;
    foreach(target_entry_cursor, key_traceprov_targets){
        TraceProvTarget *target = (TraceProvTarget *)(lfirst(target_entry_cursor));
        target->sublinks = lappend(target->sublinks, makeTraceProvTargetSublinkItem(layer_number, foreach_current_index(target_entry_cursor)));
    }
}

TraceProvWindowFrameEntry traceprov_make_window_frame_entry(
    TraceProvEntryKind kind,
    TraceProvLayerNumber log_layer_number
){
    TraceProvWindowFrameEntry *window_entry = palloc0_object(TraceProvWindowFrameEntry);
    window_entry->kind = kind;
    window_entry->log_layer_number = log_layer_number;
    return window_entry;
}

TraceProvTarget *makeTraceProvTarget(
    bool isPointer, 
    TargetEntry *targetEntry,
    TraceProvDependency *dependency,
    int setNumber,
    bool isSetPointer,
    List *sublinks,
    TraceProvWindowFrameEntry *window_entry
){
    TraceProvTarget *tpTarget = palloc0_object(TraceProvTarget);
    tpTarget->isPointer = isPointer;
    tpTarget->targetEntry = targetEntry;
    tpTarget->graph = dependency;
    tpTarget->setNumber = setNumber;
    tpTarget->isSetPointer = isSetPointer;
    tpTarget->sublinks = sublinks;
    tpTarget->window_entry = window_entry;
    return tpTarget;
}

void _assertIsArtificial(const TargetEntry *target){
    if (target->resorigtbl != InvalidOid || target->resorigcol != 0){
        elog(ERROR, "Expected no table info to for the pointer node");
    }
}

TraceProvEntry *traceprov_resolve_entry(
    const TraceProvTarget * tp_target, 
    List **child_graphs,
    List **exprs
){
    const TargetEntry *target = tp_target->targetEntry;
    if (exprs){
        *exprs = lappend(*exprs, target->expr);
    }
    TraceProvEntry *tp_entry = makeTraceProvEntry();
    TraceProvDependency *graph = tp_target->graph;
    if (tp_target->isSetPointer){
        tp_entry->kind = TP_ENTRY_SET_POINTER;
        _assertIsArtificial(target);
        if (tp_target->graph != NULL)
            elog(ERROR, "Expected the graph for set pointer to be null!");
        graph = NULL;
    }else{
        // In this case, we're dealing with a window entry.
        if (tp_target->window_entry != NULL){

            if (graph != NULL)
                elog(ERROR, "Expected the graph to always be null for window entry!")

            if (tp_target->window_entry->kind < TP_ENTRY_FRAME_START)
                elog(ERROR, "Got invalid window entry kind!");

            tp_entry->kind = tp_target->window_entry->kind;
            tp_entry->window_entry.log_layer_number = tp_target->window_entry->log_layer_number;
        }
        if (graph != NULL){
            if (target->resorigtbl != InvalidOid || target->resorigcol != 0){
                elog(ERROR, "Expected no table info to for the pointer node");
            }
            tp_entry->kind = TP_ENTRY_KIND_POINTER;
        }else{
            tp_entry->kind = TP_ENTRY_KIND_BASE_RELATION;
            tp_entry->relId = target->resorigtbl;
            tp_entry->resNo = target->resno;
            tp_entry->attrNumber = target->resorigcol;
        }
        // Because there can be multiple graphs for the set. so, this makes things nicer.
        if (tp_target->setNumber > 0) graph = TRACEPROV_SET_GRAPH;
    }
    if (child_graphs){
        *child_graphs = lappend(*child_graphs, graph);
    }
    tp_entry->setNumber = tp_target->setNumber;
    tp_entry->sublinks = tp_target->sublinks;
    return tp_entry;
}

TraceProvEntry *makeTraceProvEntry(){
    return palloc0_object(TraceProvEntry);
}

TraceProvDependency *make_traceprov_dependency(
    TraceProvGraphKind kind,
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
    } else if (entry->kind == TP_ENTRY_SET_POINTER){
        kindStr = "TP_ENTRY_SET_POINTER";
    }else{
        elog(ERROR, "Got invalid kind: %d", entry->kind);
    }
    
    StringInfoData sublink_items;
    initStringInfo(&sublink_items);
    appendStringInfoChar(&sublink_items, '[');
    ListCell *sublink_item_cursor;
    foreach(sublink_item_cursor, entry->sublinks){
        if (NEED_SEP(sublink_item_cursor)){
            appendStringInfoChar(&sublink_items, ',');
        }
        const TraceProvTargetSublinkItem *sublink_item = (TraceProvTargetSublinkItem *)(lfirst(sublink_item_cursor));
        appendStringInfo(&sublink_items, "(%d, %d)", sublink_item->layer_number, sublink_item->offset_in_key);
    }
    appendStringInfoChar(&sublink_items, ']');
    appendStringInfo(
        &buf, 
        "[TraceProvEntry (kind: %s, relid: %d, resno: %d, attrNumber: %d, setNumber: %d, sublinks: %s)]",
        kindStr,
        entry->relId,
        entry->resNo,
        entry->attrNumber,
        entry->setNumber,
        sublink_items.data
    );
    return buf.data;
}

void appendIndentAware(StringInfoData *buf, int indentCount){
    for (int i = 0; i < indentCount; i++){
        appendStringInfo(buf, "\t");
    }
}

char *traceProvParseContextToJson(const TraceProvParseContext *context){
    StringInfoData buf;
    initStringInfo(&buf);
    appendStringInfo(&buf, "{");
    appendStringInfo(&buf, "\"global_layer_number\": %d", context->global_layer_number);
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"unique_idx\": %lld", context->unique_idx);
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"simple_incrementor\": %d", context->simple_incrementor);
    appendStringInfo(&buf, ",");
    ListCell *setPaddingMapItemCursor;
    appendStringInfo(&buf, "\"setPaddingMap\": [");
    foreach(setPaddingMapItemCursor, context->properties->setPaddingMap){
        if (NEED_SEP(setPaddingMapItemCursor)){
            appendStringInfo(&buf, ",");
        }
        const TraceProvSetPaddingMapItem *setPaddingMapItem = (TraceProvSetPaddingMapItem *)lfirst(setPaddingMapItemCursor);
        appendStringInfo(&buf, "{\"setNumber\": %d, \"padding\": %d}", setPaddingMapItem->setNumber, setPaddingMapItem->padding);
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"setGraphMap\": [");
    ListCell *setGraphMapItemCursor;
    foreach(setGraphMapItemCursor, context->properties->setGraphMap){
        if (NEED_SEP(setGraphMapItemCursor)){
            appendStringInfo(&buf, ",");
        }
        const TraceProvSetGraphMapItem *setGraphMapItem = (TraceProvSetGraphMapItem *)lfirst(setGraphMapItemCursor);
        appendStringInfo(&buf, "{\"setNumber\": %d, \"graph\": %s}", setGraphMapItem->setNumber, traceProvDependencyToJson(setGraphMapItem->graph));
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"sublinks\": [");
    ListCell *sublinkGraphCursor;
    foreach(sublinkGraphCursor, context->properties->sublinkMap){
        if(NEED_SEP(sublinkGraphCursor)){
            appendStringInfo(&buf, ",");
        }
        appendStringInfo(&buf, "%s", traceProvDependencyToJson((TraceProvDependency *)lfirst(sublinkGraphCursor)));
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, "}");
    return buf.data;
}

// Makes things easier, ngl.
// The output can then be used on other formats too (like dot graphs)
char *traceProvDependencyToJson(const TraceProvDependency *graph){
    StringInfoData buf;
    initStringInfo(&buf);
    appendStringInfo(&buf, "{");
    appendStringInfo(&buf, "\"graphType\":");
    if (graph == NULL){
        appendStringInfo(&buf, "\"NULL\"");
    }else if (graph == TRACEPROV_SET_GRAPH){
        appendStringInfo(&buf, "\"SET_GRAPH\"");
    }else{
        appendStringInfo(&buf, "\"REGULAR\"");
        appendStringInfo(&buf, ",");
        appendStringInfo(&buf, "\"headNumber\": %d,", graph->headNumber);
        appendStringInfo(&buf, "\"entries\": [");
        ListCell *entryCursor;
        bool needsSep = false;
        foreach(entryCursor, graph->entries){
            if (needsSep){
                appendStringInfo(&buf, ",");
            }
            needsSep = true;
            TraceProvEntry *entry = (TraceProvEntry*)lfirst(entryCursor);
            appendStringInfo(&buf, "\"%s\"", serializeTraceProvEntry(entry));
        }
        appendStringInfo(&buf, "],");
        ListCell *childCursor;
        appendStringInfo(&buf, "\"children\": [");
        needsSep = false;
        foreach(childCursor, graph->children){
            if (needsSep){
                appendStringInfo(&buf, ",");
            }
            needsSep = true;
            TraceProvDependency *child = (TraceProvDependency *)lfirst(childCursor);
            appendStringInfo(&buf, "%s", traceProvDependencyToJson(child));
        }
        appendStringInfo(&buf, "]");
    }
    appendStringInfo(&buf, "}");
    return buf.data;
}

char * traceprovDependencyToString(int indent, const TraceProvDependency *graph){
    if (graph == NULL){
        return pstrdup("<NULL>");
    } else if (graph == TRACEPROV_SET_GRAPH){
        return pstrdup("<UNION SET GRAPH>");
    }
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

void traceprovPrintDependency(const TraceProvDependency *graph, const TraceProvParseContext *context){
    elog(INFO, "TraceProvDependency: ");
    elog(INFO, "\n%s", traceprovDependencyToString(0, graph));
    elog(INFO, "JSON: %s", traceProvDependencyToJson(graph));
    traceprovPrintContext(context);
}

void traceprovPrintContext(const TraceProvParseContext *context){
    elog(INFO, "TraceProvDependency Parse Context: ");
    elog(INFO, "JSON: %s", traceProvParseContextToJson(context));
}

typedef struct TraceProvDependencyMetaHeader {
    TraceProvLayerNumber max_layer_number;
    // Number of graphs being stored.
    uint32 num_graphs;
    uint32 num_set_padding_map_items;
    uint32 num_set_graph_map_items;
    uint32 num_sublink_items;
} TraceProvDependencyMetaHeader;

static_assert(sizeof(TraceProvDependencyMetaHeader) == 20);

// This is not in the header for a reason, nothing outside of this file
// should know that this even exists.
typedef struct TraceProvDependencyHeader {
    uint32 idx; // own's index (each block has a unique index)
    uint32 numberOfEntries; // Number of entries
    uint32 numberOfDirectChildren; // Number of direct children.
    TraceProvDependency *graphPtr; // Useful to detect if the graph is null or not.
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

void _serializeContext(const TraceProvParseContext*, FILE *);
TraceProvDependency *_deserializeTraceProvDependency(FILE *);
TraceProvParseContext *_deserializeTraceProvParseContext(FILE *, TraceProvDependencyMetaHeader*);

// Serializes multiple graphs in the graph file.
// This needs to be a list, because we can multiple pointers.
// Need to also dump some things from the context. For example, need to dump the set-padding map.
void serializeTraceProvDepedency(List *graphs, TraceProvParseContext *context){
    ListCell *graphCursor;
    const char *dumpPath = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dumpPath, "wb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    TraceProvDependencyMetaHeader meta_header;
    meta_header.max_layer_number = context->global_layer_number;
    meta_header.num_graphs = list_length(graphs);
    meta_header.num_set_padding_map_items = list_length(context->properties->setPaddingMap);
    meta_header.num_set_graph_map_items = list_length(context->properties->setGraphMap);
    meta_header.num_sublink_items = list_length(context->properties->sublinkMap);
    failSafeWrite(fptr, &meta_header, sizeof(TraceProvDependencyMetaHeader));
    _serializeContext(context, fptr);
    foreach(graphCursor, graphs){
        const TraceProvDependency *graph = (TraceProvDependency *)(lfirst(graphCursor));
        elog(INFO, "Serializing: ");
        traceprovPrintDependency(graph, context);
        _serializeTraceProvDepedency(graph, fptr);
    }
    fclose(fptr);
}

typedef struct TraceProvEntryMetaHeader {
    uint32 num_keys;
} TraceProvEntryMetaHeader;

// Used for serialization, since we also need to serialize the corresponding keys in sublinks.
static void _serializeTraceProvEntry(const TraceProvEntry *entry, FILE *output_file){
    TraceProvEntryMetaHeader meta_header;
    meta_header.num_keys = list_length(entry->sublinks);
    failSafeWrite(output_file, &meta_header, sizeof(TraceProvEntryMetaHeader));
    failSafeWrite(output_file, entry, sizeof(TraceProvEntry));
    ListCell *entry_cursor;
    foreach(entry_cursor, entry->sublinks){
        TraceProvTargetSublinkItem *sublink_item = (TraceProvTargetSublinkItem *)lfirst(entry_cursor);
        failSafeWrite(output_file, sublink_item, sizeof(TraceProvTargetSublinkItem));
    }
}


static TraceProvEntry *_deserializeTraceProvEntry(FILE *input_file){
    TraceProvEntry *entry = palloc0_object(TraceProvEntry);
    TraceProvEntryMetaHeader meta_header;
    failSafeRead(input_file, &meta_header, sizeof(TraceProvEntryMetaHeader));
    failSafeRead(input_file, entry, sizeof(TraceProvEntry));
    entry->sublinks = NIL;
    for (uint32 i = 0; i < meta_header.num_keys; i++){
        TraceProvTargetSublinkItem *sublink_item = palloc0_object(TraceProvTargetSublinkItem);
        failSafeRead(input_file, sublink_item, sizeof(TraceProvTargetSublinkItem));
        entry->sublinks = lappend(entry->sublinks, sublink_item);
    }
    return entry;
}

void _serializeTraceProvDepedency(const TraceProvDependency *graph, FILE *output_file){

    TraceProvDependencyHeader *header = palloc0_object(TraceProvDependencyHeader);
    if (TRACEPROV_GRAPH_IS_VALID(graph)){
        header->numberOfEntries = list_length(graph->entries);
        header->numberOfDirectChildren = list_length(graph->children);
    }
    memcpy(&header->graphPtr, &graph, sizeof(TraceProvDependency *));
    failSafeWrite(output_file, header, sizeof(TraceProvDependencyHeader));
    if (TRACEPROV_GRAPH_IS_VALID(graph)){
        failSafeWrite(output_file, &graph->headNumber, sizeof(graph->headNumber));
        ListCell *entryCursor;
        // Write the entries next.
        foreach(entryCursor, graph->entries){
            const TraceProvEntry *entry = (TraceProvEntry *)lfirst(entryCursor);
            _serializeTraceProvEntry(entry, output_file);
        }

        ListCell *childCursor;
        foreach(childCursor, graph->children){
            const TraceProvDependency *childGraph = (TraceProvDependency *)lfirst(childCursor);
            _serializeTraceProvDepedency(childGraph, output_file);
        }
    }
}

// Dumps the properties in the file.
void _serializeContext(const TraceProvParseContext* context, FILE* file){
    ListCell *paddingMapItemCursor;
    foreach(paddingMapItemCursor, context->properties->setPaddingMap){
        TraceProvSetPaddingMapItem *paddingMapItem = (TraceProvSetPaddingMapItem *)lfirst(paddingMapItemCursor);
        failSafeWrite(file, paddingMapItem, sizeof(TraceProvSetPaddingMapItem));
    }

    ListCell *graphMapItemCursor;
    foreach(graphMapItemCursor, context->properties->setGraphMap){
        TraceProvSetGraphMapItem *graphMapItem = (TraceProvSetGraphMapItem *)lfirst(graphMapItemCursor);
        // To make serialization easier, it gets wrapped in one graph.
        // During deserialization, it gets unwrapped.
        const TraceProvDependency *wrapperDependency = graphMapItem->graph;
        failSafeWrite(file, graphMapItem, sizeof(TraceProvSetGraphMapItem));
        _serializeTraceProvDepedency(wrapperDependency, file);
    }
    ListCell *sublinkMapItemCursor;
    foreach(sublinkMapItemCursor, context->properties->sublinkMap){
        const TraceProvDependency *sublinkGraph = (TraceProvDependency *)lfirst(sublinkMapItemCursor);
        _serializeTraceProvDepedency(sublinkGraph, file);
    }
}

// Returns list of deserialized graphs.
List* deserializeTraceProvDependency(TraceProvParseContext **parsedContext){
    const char *dumpPath = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dumpPath, "rb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    TraceProvDependencyMetaHeader metaHeader;
    failSafeRead(fptr, &metaHeader, sizeof(TraceProvDependencyMetaHeader));
    if (metaHeader.num_graphs < 0){
        elog(ERROR, "Got invalid number of graphs in the file: %d", metaHeader.num_graphs);
    }
    *parsedContext = _deserializeTraceProvParseContext(fptr, &metaHeader);
    List *graphs = NIL;
    for (int i = 0; i < metaHeader.num_graphs; i++){
        TraceProvDependency*graph = _deserializeTraceProvDependency(fptr);
        elog(INFO, "Deserializing: ");
        traceprovPrintDependency(graph, *parsedContext);
        graphs = lappend(graphs, graph);
    }

    fclose(fptr);
    return graphs;
}

TraceProvDependency *_deserializeTraceProvDependency(FILE *file){
    TraceProvDependencyHeader header;
    failSafeRead(file, &header, sizeof(TraceProvDependencyHeader));
    if (TRACEPROV_GRAPH_IS_VALID(header.graphPtr)){
        TraceProvDependency *graph = palloc0_object(TraceProvDependency);
        failSafeRead(file, &graph->headNumber, sizeof(graph->headNumber));

        List *entries = NIL;
        for (uint32 idx = 0; idx < header.numberOfEntries; idx++){
            TraceProvEntry *entry  = _deserializeTraceProvEntry(file);
            entries = lappend(entries, entry);
        }

        List *children = NIL;
        for (uint32 idx = 0; idx < header.numberOfDirectChildren; idx++){
            children = lappend(children, (void*) _deserializeTraceProvDependency(file));
        }
        
        graph->children = children;
        graph->entries = entries;
        return graph;
    }else{
        return header.graphPtr;
    }
}

// Deserialize some of the parse context fields.
// Doesn't do all the fields, but only the ones necessary (like the set->graph, and set->padding maps)
TraceProvParseContext *_deserializeTraceProvParseContext(FILE *file, TraceProvDependencyMetaHeader* metaHeader){
    TraceProvParseContext *context = palloc0_object(TraceProvParseContext);
    tpParseInitializeContext(context);
    for (uint32 i = 0; i < metaHeader->num_set_padding_map_items; i++){
        TraceProvSetPaddingMapItem *setPaddingMapItem = palloc0_object(TraceProvSetPaddingMapItem);
        failSafeRead(file, setPaddingMapItem, sizeof(TraceProvSetPaddingMapItem));
        context->properties->setPaddingMap = lappend(context->properties->setPaddingMap, setPaddingMapItem);
    }

    for (uint32 i = 0; i < metaHeader->num_set_graph_map_items; i++){
        TraceProvSetGraphMapItem *setGraphMapItem = palloc0_object(TraceProvSetGraphMapItem);
        failSafeRead(file, setGraphMapItem, sizeof(TraceProvSetGraphMapItem));
        TraceProvDependency *wrapper = _deserializeTraceProvDependency(file);
        setGraphMapItem->graph = wrapper;
        context->properties->setGraphMap = lappend(context->properties->setGraphMap, setGraphMapItem);
    }
    for (uint32 i = 0; i < metaHeader->num_sublink_items; i++){
        TraceProvDependency *graph = _deserializeTraceProvDependency(file);
        context->properties->sublinkMap = lappend(context->properties->sublinkMap, graph);
    }
    return context;
}

TraceProvParseContext *traceprov_shallow_copy_context(const TraceProvParseContext *context){
    TraceProvParseContext *copied_context = palloc0_object(TraceProvParseContext);
    memcpy(copied_context, context, sizeof(TraceProvParseContext));
    copied_context->parent_targets = list_copy(context->parent_targets);
    copied_context->root_context = context->root_context;
    return copied_context;
}

void tp_add_aggregate_property(const TraceProvParseContext *context, const Agg *agg, TraceProvLayerNumber layer_number){
    TraceProvParseContext *root_context = GET_ROOT_CONTEXT(context);
    ListCell *cursor;
    TraceProvAggregateProperty *agg_property = NULL;
    // Try finding an existing aggregate property (will happen when this is split)
    foreach(cursor, root_context->properties->aggregate_properties){
        TraceProvAggregateProperty *candidate = (TraceProvAggregateProperty *)(lfirst(cursor));
        if (candidate->layer_number == layer_number){
            agg_property = candidate;
            break;
        }
    }
    if (agg_property != NULL && agg->aggsplit == AGGSPLIT_SIMPLE){
        elog(ERROR, "Didn't expect to find the aggregate again (since it is not split)");
    }
    if (agg_property == NULL){
        agg_property = palloc0_object(TraceProvAggregateProperty);
        agg_property->initial_strategy = -1;
        agg_property->combine_strategy = -1;
        // Add this here, so that we don't remember that this was new property.
        root_context->properties->aggregate_properties = lappend(root_context->properties->aggregate_properties, agg_property);
    }
    if (agg->aggsplit == AGGSPLIT_SIMPLE || agg->aggsplit == AGGSPLIT_INITIAL_SERIAL){
        if (agg_property->initial_strategy != -1){
            elog(ERROR, "Trying to overwrite intial strategy field!");
        }
        agg_property->initial_strategy = agg->aggstrategy;
    } else if (agg->aggsplit == AGGSPLIT_FINAL_DESERIAL){
        if (agg_property->combine_strategy != -1){
            elog(ERROR, "Trying to overwrite combine strategy field!");
        }
        agg_property->combine_strategy = agg->aggstrategy;
    }else{
        elog(ERROR, "Unrecognized agg split field!");
    }
}