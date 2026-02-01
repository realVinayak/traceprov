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

void tp_parse_initialize_context(TraceProvParseContext *context){
    // The first layer is 1.
    context->global_layer_number = 1;
    context->unique_idx = 0;
    context->simple_incrementor = 0;
    context->properties = palloc0_object(TraceProvParseGraphProperties);
    context->properties->set_padding_map = NIL;
    context->properties->set_graph_map = NIL;
    context->properties->sublink_map = NIL;
    context->properties->set_pointer_map = NIL;
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
    GET_ROOT_CONTEXT(context)->properties->set_padding_map = lappend(GET_ROOT_CONTEXT(context)->properties->set_padding_map, mapItem);
}

void tp_add_set_graph_item(TraceProvParseContext *context, int setNumber, TraceProvDependency*graph){
    TraceProvSetGraphMapItem *setGraphMapItem = palloc0_object(TraceProvSetGraphMapItem);
    setGraphMapItem->graph = graph;
    setGraphMapItem->setNumber = setNumber;
    GET_ROOT_CONTEXT(context)->properties->set_graph_map = lappend(GET_ROOT_CONTEXT(context)->properties->set_graph_map, setGraphMapItem);
}

TraceProvTargetSublinkItem *makeTraceProvTargetSublinkItem(
    TraceProvLayerNumber layer_number,
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
    foreach(target_entry_cursor, list_concat_copy(key_traceprov_targets, ptr_traceprov_targets)){
        TraceProvEntry *tpEntry = traceprov_resolve_entry(((TraceProvTarget *)lfirst(target_entry_cursor)), &child_graphs, NULL);
        entries = lappend(entries, tpEntry);
    }
    const TraceProvDependency *graph = make_traceprov_dependency(
        TP_LOG,
        layer_number,
        child_graphs,
        entries
    );
    GET_ROOT_CONTEXT(context)->properties->sublink_map = lappend(GET_ROOT_CONTEXT(context)->properties->sublink_map, (TraceProvDependency*)graph);
    target_entry_cursor = NULL;
    foreach(target_entry_cursor, key_traceprov_targets){
        TraceProvTarget *target = (TraceProvTarget *)(lfirst(target_entry_cursor));
        target->sublinks = lappend(target->sublinks, makeTraceProvTargetSublinkItem(layer_number, foreach_current_index(target_entry_cursor)));
    }
}

TraceProvWindowFrameEntry *traceprov_make_window_frame_entry(
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
    TraceProvWindowFrameEntry *window_entry,
    bool is_pointer_for_window
){
    TraceProvTarget *tpTarget = palloc0_object(TraceProvTarget);
    tpTarget->isPointer = isPointer;
    tpTarget->targetEntry = targetEntry;
    tpTarget->graph = dependency;
    tpTarget->setNumber = setNumber;
    tpTarget->isSetPointer = isSetPointer;
    tpTarget->sublinks = sublinks;
    tpTarget->window_entry = window_entry;
    tpTarget->is_pointer_for_window = is_pointer_for_window;
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
                elog(ERROR, "Expected the graph to always be null for window entry!");

            if (tp_target->window_entry->kind < TP_ENTRY_FRAME_START)
                elog(ERROR, "Got invalid window entry kind!");

            tp_entry->kind = tp_target->window_entry->kind;
            // TODO: This is redundant...
            tp_entry->window_entry.kind = tp_target->window_entry->kind;
            tp_entry->window_entry.log_layer_number = tp_target->window_entry->log_layer_number;
        } else if (graph != NULL){
            _assertIsArtificial(tp_target->targetEntry);
            tp_entry->kind = TP_ENTRY_KIND_POINTER;
            tp_entry->is_pointer_for_window = tp_target->is_pointer_for_window;
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
    graphNode->graph_type = kind;
    graphNode->headNumber = headNumber;
    graphNode->children = children;
    graphNode->entries = entries;
    return graphNode;
}

static char *tp_entry_serialize_sublinks(List *sublinks){
    StringInfoData sublink_items;
    initStringInfo(&sublink_items);
    appendStringInfoChar(&sublink_items, '[');
    ListCell *sublink_item_cursor;
    foreach(sublink_item_cursor, sublinks){
        if (NEED_SEP(sublink_item_cursor)){
            appendStringInfoChar(&sublink_items, ',');
        }
        const TraceProvTargetSublinkItem *sublink_item = (TraceProvTargetSublinkItem *)(lfirst(sublink_item_cursor));
        appendStringInfo(&sublink_items, "(%d, %d)", sublink_item->layer_number, sublink_item->offset_in_key);
    }
    appendStringInfoChar(&sublink_items, ']');
    return sublink_items.data;
}

static char *tp_entry_serialize_kind(TraceProvEntryKind kind){
    char *kind_str = "TP_ENTRY_INVALID";
    switch (kind) {
        case TP_ENTRY_KIND_BASE_RELATION:
            kind_str = "TP_ENTRY_KIND_BASE_RELATION";
            break;
        case TP_ENTRY_KIND_POINTER:
            kind_str = "TP_ENTRY_KIND_POINTER";
            break;
        case TP_ENTRY_SET_POINTER:
            kind_str = "TP_ENTRY_SET_POINTER";
            break;
        case TP_ENTRY_FRAME_START:
            kind_str = "TP_ENTRY_FRAME_START";
            break;
        case TP_ENTRY_FRAME_END:
            kind_str = "TP_ENTRY_FRAME_END";
            break;
        case TP_ENTRY_FRAME_INHERIT:
            kind_str = "TP_ENTRY_FRAME_INHERIT";
            break;
        default:
            elog(ERROR, "Got invalid kind: %d", kind);
    }
    return pstrdup(kind_str);
}

static char *tp_entry_serialize_window(TraceProvWindowFrameEntry *entry){
    StringInfoData window_data;
    initStringInfo(&window_data);
    appendStringInfoChar(&window_data, '[');
    if (entry->kind != 0 && entry->kind < TP_ENTRY_FRAME_START)
        elog(ERROR, "Got invalid window entry state");
    if (entry->kind >= TP_ENTRY_FRAME_START){
        char *kind_str = tp_entry_serialize_kind(entry->kind);
        appendStringInfo(&window_data, "Window(kind: %s, layer_number: %d)", kind_str, entry->log_layer_number);
    }
    appendStringInfoChar(&window_data, ']');
    return window_data.data;
}

static char *serializeTraceProvEntry(TraceProvEntry *entry){
    StringInfoData buf;
    initStringInfo(&buf);
    char *kindStr = tp_entry_serialize_kind(entry->kind);

    appendStringInfo(
        &buf, 
        "[TraceProvEntry (kind: %s, relid: %d, resno: %d, attrNumber: %d, setNumber: %d, sublinks: %s, window: %s, is_ptr_for_window: %d)]",
        kindStr,
        entry->relId,
        entry->resNo,
        entry->attrNumber,
        entry->setNumber,
        tp_entry_serialize_sublinks(entry->sublinks),
        tp_entry_serialize_window(&entry->window_entry),
        entry->is_pointer_for_window
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
    ListCell *set_padding_map_item_cursor;
    appendStringInfo(&buf, "\"setPaddingMap\": [");
    foreach(set_padding_map_item_cursor, context->properties->set_padding_map){
        if (NEED_SEP(set_padding_map_item_cursor)){
            appendStringInfo(&buf, ",");
        }
        const TraceProvSetPaddingMapItem *setPaddingMapItem = (TraceProvSetPaddingMapItem *)lfirst(set_padding_map_item_cursor);
        appendStringInfo(&buf, "{\"setNumber\": %d, \"padding\": %d}", setPaddingMapItem->setNumber, setPaddingMapItem->padding);
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"setGraphMap\": [");
    ListCell *set_graph_map_item_cursor;
    foreach(set_graph_map_item_cursor, context->properties->set_graph_map){
        if (NEED_SEP(set_graph_map_item_cursor)){
            appendStringInfo(&buf, ",");
        }
        const TraceProvSetGraphMapItem *setGraphMapItem = (TraceProvSetGraphMapItem *)lfirst(set_graph_map_item_cursor);
        appendStringInfo(&buf, "{\"setNumber\": %d, \"graph\": %s}", setGraphMapItem->setNumber, traceProvDependencyToJson(setGraphMapItem->graph));
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"sublinks\": [");
    ListCell *sublink_graph_cursor;
    foreach(sublink_graph_cursor, context->properties->sublink_map){
        if(NEED_SEP(sublink_graph_cursor)){
            appendStringInfo(&buf, ",");
        }
        appendStringInfo(&buf, "%s", traceProvDependencyToJson((TraceProvDependency *)lfirst(sublink_graph_cursor)));
    }
    appendStringInfo(&buf, "]");
    appendStringInfo(&buf, ",");
    appendStringInfo(&buf, "\"set_pointer_item\": [");
    ListCell *set_pointer_item_cursor;
    foreach(set_pointer_item_cursor, context->properties->set_pointer_map){
        if (NEED_SEP(set_pointer_item_cursor))
            appendStringInfo(&buf, ",");
        const TraceProvSetPointerItem *item = (TraceProvSetPointerItem *)lfirst(set_pointer_item_cursor);
        appendStringInfo(&buf, "{");
        appendStringInfo(&buf, "\"set_pointer\": %d", item->set_pointer);
        appendStringInfo(&buf, ",");
        appendStringInfo(&buf, "\"refs\": [");
        ListCell *ref_cursor;
        foreach(ref_cursor, item->refs){
            if (NEED_SEP(ref_cursor))
                appendStringInfo(&buf, ",");
            appendStringInfo(&buf, "%d", lfirst_int(ref_cursor));
        }
        appendStringInfo(&buf, "]");
        appendStringInfo(&buf, "}");
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
        switch (graph->graph_type){
            case TP_AGGREGATE:
                appendStringInfo(&buf, "\"AGGREGATE\"");
                break;
            case TP_LOG:
                appendStringInfo(&buf, "\"LOG\"");
                break;
            default:
                elog(ERROR, "Got unexpected graph type: %d", graph->graph_type);
                break;
        }
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
    appendStringInfo(&buf, "\tgraph_type: %d\n", graph->graph_type);
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
    uint32 num_set_pointer_map;
} TraceProvDependencyMetaHeader;

static_assert(sizeof(TraceProvDependencyMetaHeader) == 24);

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

#define FAIL_SAFE_WRITE_INT(FILE, VALUE, TYPE) do { \
    int32 value = VALUE; \
    if (sizeof(TYPE) != sizeof(int32)) {  \
        elog(ERROR, "macro only for int!"); \
    } \
    failSafeWrite(FILE, &value, sizeof(int32)); \
} while(0); \

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

typedef struct TraceProvStringHeader {
    size_t size;
} TraceProvStringHeader;
// Serializes multiple graphs in the graph file.
// This needs to be a list, because we can multiple pointers.
// Need to also dump some things from the context. For example, need to dump the set-padding map.
// Also dumps the parsed back. Useful for debugging things layer + tests.
void serializeTraceProvDepedency(List *graphs, TraceProvParseContext *context, const char *parsed_back_query){
    ListCell *graphCursor;
    const char *dump_path = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dump_path, "wb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    TraceProvDependencyMetaHeader meta_header;
    meta_header.max_layer_number = context->global_layer_number;
    meta_header.num_graphs = list_length(graphs);
    meta_header.num_set_padding_map_items = list_length(context->properties->set_padding_map);
    meta_header.num_set_graph_map_items = list_length(context->properties->set_graph_map);
    meta_header.num_sublink_items = list_length(context->properties->sublink_map);
    meta_header.num_set_pointer_map = list_length(context->properties->set_pointer_map);
    failSafeWrite(fptr, &meta_header, sizeof(TraceProvDependencyMetaHeader));
    _serializeContext(context, fptr);
    foreach(graphCursor, graphs){
        const TraceProvDependency *graph = (TraceProvDependency *)(lfirst(graphCursor));
        elog(INFO, "Serializing: ");
        traceprovPrintDependency(graph, context);
        _serializeTraceProvDepedency(graph, fptr);
    }

    TraceProvStringHeader string_header;
    if (parsed_back_query == NULL) {
        string_header.size = 0;
    }else{
        string_header.size = strlen(parsed_back_query);
    }

    failSafeWrite(fptr, &string_header, sizeof(TraceProvStringHeader));
    if (string_header.size) failSafeWrite(fptr, parsed_back_query, string_header.size);
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
        failSafeWrite(output_file, &graph->graph_type, sizeof(graph->graph_type));
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
    ListCell *padding_map_item_cursor;
    foreach(padding_map_item_cursor, context->properties->set_padding_map){
        TraceProvSetPaddingMapItem *padding_map_item = (TraceProvSetPaddingMapItem *)lfirst(padding_map_item_cursor);
        failSafeWrite(file, padding_map_item, sizeof(TraceProvSetPaddingMapItem));
    }

    ListCell *graph_map_item_cursor;
    foreach(graph_map_item_cursor, context->properties->set_graph_map){
        TraceProvSetGraphMapItem *graph_map_item = (TraceProvSetGraphMapItem *)lfirst(graph_map_item_cursor);
        // To make serialization easier, it gets wrapped in one graph.
        // During deserialization, it gets unwrapped.
        const TraceProvDependency *wrapperDependency = graph_map_item->graph;
        failSafeWrite(file, graph_map_item, sizeof(TraceProvSetGraphMapItem));
        _serializeTraceProvDepedency(wrapperDependency, file);
    }
    ListCell *sublink_map_item_cursor;
    foreach(sublink_map_item_cursor, context->properties->sublink_map){
        const TraceProvDependency *sublink_graph = (TraceProvDependency *)lfirst(sublink_map_item_cursor);
        _serializeTraceProvDepedency(sublink_graph, file);
    }
    
    ListCell *set_pointer_cursor;
    foreach(set_pointer_cursor, context->properties->set_pointer_map){
        const TraceProvSetPointerItem *set_pointer_item = (TraceProvSetPointerItem *)lfirst(set_pointer_cursor);
        FAIL_SAFE_WRITE_INT(file, set_pointer_item->set_pointer, int32);
        FAIL_SAFE_WRITE_INT(file, list_length(set_pointer_item->refs), int);
        ListCell *ref_cursor;
        foreach(ref_cursor, set_pointer_item->refs){
            FAIL_SAFE_WRITE_INT(file, lfirst_int(ref_cursor), int);
        }
    }

}

// Returns list of deserialized graphs.
List* deserializeTraceProvDependency(TraceProvParseContext **parsed_context, char **parsed_back_query){
    const char *dump_path = psprintf(TRACEPROV_GRAPH_FILE,  DataDir);
    FILE *fptr = fopen(dump_path, "rb"); 
    if (fptr == NULL) {
        elog(ERROR, "Error opening file for dumping graph!");
    }
    TraceProvDependencyMetaHeader metaHeader;
    failSafeRead(fptr, &metaHeader, sizeof(TraceProvDependencyMetaHeader));
    if (metaHeader.num_graphs < 0){
        elog(ERROR, "Got invalid number of graphs in the file: %d", metaHeader.num_graphs);
    }
    *parsed_context = _deserializeTraceProvParseContext(fptr, &metaHeader);
    List *graphs = NIL;
    for (int i = 0; i < metaHeader.num_graphs; i++){
        TraceProvDependency*graph = _deserializeTraceProvDependency(fptr);
        elog(INFO, "Deserializing: ");
        traceprovPrintDependency(graph, *parsed_context);
        graphs = lappend(graphs, graph);
    }

    if (parsed_back_query){
        TraceProvStringHeader string_header;
        failSafeRead(fptr, &string_header, sizeof(TraceProvStringHeader));
        if (string_header.size){
            char *parsed_back_holder = palloc0(string_header.size + 1);
            failSafeRead(fptr, parsed_back_holder, string_header.size);
            *parsed_back_query = parsed_back_holder;
        }
    }


    fclose(fptr);
    return graphs;
}

TraceProvDependency *_deserializeTraceProvDependency(FILE *file){
    TraceProvDependencyHeader header;
    failSafeRead(file, &header, sizeof(TraceProvDependencyHeader));
    if (TRACEPROV_GRAPH_IS_VALID(header.graphPtr)){
        TraceProvDependency *graph = palloc0_object(TraceProvDependency);
        failSafeRead(file, &graph->graph_type, sizeof(graph->graph_type));
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
    tp_parse_initialize_context(context);
    context->root_context = context;
    for (uint32 i = 0; i < metaHeader->num_set_padding_map_items; i++){
        TraceProvSetPaddingMapItem *setPaddingMapItem = palloc0_object(TraceProvSetPaddingMapItem);
        failSafeRead(file, setPaddingMapItem, sizeof(TraceProvSetPaddingMapItem));
        context->properties->set_padding_map = lappend(context->properties->set_padding_map, setPaddingMapItem);
    }

    for (uint32 i = 0; i < metaHeader->num_set_graph_map_items; i++){
        TraceProvSetGraphMapItem *setGraphMapItem = palloc0_object(TraceProvSetGraphMapItem);
        failSafeRead(file, setGraphMapItem, sizeof(TraceProvSetGraphMapItem));
        TraceProvDependency *wrapper = _deserializeTraceProvDependency(file);
        setGraphMapItem->graph = wrapper;
        context->properties->set_graph_map = lappend(context->properties->set_graph_map, setGraphMapItem);
    }
    for (uint32 i = 0; i < metaHeader->num_sublink_items; i++){
        TraceProvDependency *graph = _deserializeTraceProvDependency(file);
        context->properties->sublink_map = lappend(context->properties->sublink_map, graph);
    }
    for (uint32 i = 0; i < metaHeader->num_set_pointer_map; i++){
        uint32 set_pointer_value = 0;
        failSafeRead(file, &set_pointer_value, sizeof(int32));
        uint32 set_ref_length = 0;
        failSafeRead(file, &set_ref_length, sizeof(int));
        for (uint32 ref_idx = 0; ref_idx < set_ref_length; ref_idx++){
            uint32 ref_value = 0;
            failSafeRead(file, &ref_value, sizeof(int));
            tp_add_set_pointer_property(context, set_pointer_value, ref_value);
        }
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

TraceProvDependency *tp_get_sublink_graph(const TraceProvParseContext *parsed_context, TraceProvLayerNumber graph_number){
    ListCell *graph_cursor;
    foreach(graph_cursor, GET_ROOT_CONTEXT(parsed_context)->properties->sublink_map){
        TraceProvDependency *dependency = (TraceProvDependency *)lfirst(graph_cursor);
        if (dependency->headNumber == graph_number)
            return dependency;
    }
    elog(ERROR, "Expected to always find the sublink!");
    return NULL;
}

TraceProvDependency *tp_get_set_graph(const TraceProvParseContext *parsed_context, const int set_number){
    ListCell *graph_cursor;
    foreach(graph_cursor, GET_ROOT_CONTEXT(parsed_context)->properties->set_graph_map){
        TraceProvSetGraphMapItem *set_graph_map_item = (TraceProvSetGraphMapItem *)lfirst(graph_cursor);
        if (set_graph_map_item->setNumber == set_number)
            return set_graph_map_item->graph;
    }
    elog(ERROR, "Expected to always find the set graph!");
    return NULL;
}

// Finds a graph in the children of a graph.
// Not recursive..
TraceProvDependency *tp_get_graph_from_children(const TraceProvDependency *graph, const TraceProvLayerNumber graph_number){
    ListCell *graph_cursor;
    foreach(graph_cursor, graph->children){
        TraceProvDependency *dependency = (TraceProvDependency *)lfirst(graph_cursor);
        if (dependency->headNumber == graph_number)
            return dependency;
    }
    elog(ERROR, "Expected to always find the graph in children!");
    return NULL;
}

void tp_add_set_pointer_property(TraceProvParseContext *context, const uint32 pointer, const uint32 ref){
    if (pointer == 0 || ref == 0)
        elog(ERROR, "Invalid args: %d, %d", pointer, ref);
    TraceProvParseContext *real_context = GET_ROOT_CONTEXT(context);
    ListCell *cursor = NULL;
    foreach(cursor, real_context->properties->set_pointer_map){
        TraceProvSetPointerItem *item = (TraceProvSetPointerItem *)lfirst(cursor);
        if (item->set_pointer == pointer)
            break;
    }
    TraceProvSetPointerItem *item = NULL;
    if (cursor == NULL){
        item = palloc0_object(TraceProvSetPointerItem);
        item->set_pointer = pointer;
        item->refs = NIL;
        real_context->properties->set_pointer_map = lappend(real_context->properties->set_pointer_map, item);
    }else{
        item = (TraceProvSetPointerItem *)lfirst(cursor);
    }
    item->refs = list_append_unique_int(item->refs, ref);
}

List *tp_get_set_pointer_property(TraceProvParseContext *context, const uint32 pointer){
    ListCell *cursor;
    foreach(cursor, GET_ROOT_CONTEXT(context)->properties->set_pointer_map){
        TraceProvSetPointerItem *item = (TraceProvSetPointerItem *)lfirst(cursor);
        if (item->set_pointer == pointer)
            return item->refs;
    }
    return NIL;
}