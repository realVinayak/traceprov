#include "derivation_utils.hpp"

extern "C" {

#include <tp_list.h>
#include "traceprov_graph.h"
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <mem_alloc.h>


static TraceProvParseContext *_deserializeTraceProvParseContext(FILE *file, TraceProvDependencyMetaHeader* metaHeader);
static TraceProvDependency *_deserializeTraceProvDependency(FILE *file);
static TraceProvEntry *_deserializeTraceProvEntry(FILE *input_file);

static TraceProvParseContext *mock_context = NULL;
static TraceProvDependency *mock_dependency = NULL;

// Returns list of deserialized graphs.
List* deserializeTraceProvDependency(
    TraceProvParseContext **parsed_context,
    char **parsed_back_query,
    const char *traceprov_graph_file,
    const bool expect_present
){
    if (mock_context || mock_dependency){
        *parsed_context = mock_context;
        return list_make1(mock_dependency);
    }
    FILE *fptr = fopen(traceprov_graph_file, "rb"); 
    if (fptr == NULL) {
        if (!expect_present){
            if (parsed_back_query){
                *parsed_back_query = NULL;
            }
            if (parsed_context){
                *parsed_context = NULL;
            }
            return NIL;
        }
        EXIT_WITH_MESSAGE("Error opening file for dumping graph!");
    }
    TraceProvDependencyMetaHeader metaHeader;
    failSafeRead(fptr, &metaHeader, sizeof(TraceProvDependencyMetaHeader));
    if (metaHeader.num_graphs < 0){
        EXIT_WITH_MESSAGE("Error opening file for dumping graph!");
    }
    *parsed_context = _deserializeTraceProvParseContext(fptr, &metaHeader);
    List *graphs = NIL;
    for (uint32_t i = 0; i < metaHeader.num_graphs; i++){
        TraceProvDependency*graph = _deserializeTraceProvDependency(fptr);
        graphs = lappend(graphs, graph);
    }

    if (parsed_back_query){
        TraceProvStringHeader string_header;
        failSafeRead(fptr, &string_header, sizeof(TraceProvStringHeader));
        if (string_header.size){
            char *parsed_back_holder = (char *)malloc(string_header.size + 1);
            memset(parsed_back_holder, 0, sizeof(string_header.size + 1));
            failSafeRead(fptr, parsed_back_holder, string_header.size);
            *parsed_back_query = parsed_back_holder;
        }
    }

    fclose(fptr);
    return graphs;
}

// Deserialize some of the parse context fields.
// Doesn't do all the fields, but only the ones necessary (like the set->graph, and set->padding maps)
TraceProvParseContext *_deserializeTraceProvParseContext(FILE *file, TraceProvDependencyMetaHeader* metaHeader){
    TraceProvParseContext *context = tp_alloc0_object(TraceProvParseContext);
    tp_parse_initialize_context(context);
    context->root_context = context;
    for (uint32 i = 0; i < metaHeader->num_set_padding_map_items; i++){
        TraceProvSetPaddingMapItem *setPaddingMapItem = tp_alloc0_object(TraceProvSetPaddingMapItem);
        failSafeRead(file, setPaddingMapItem, sizeof(TraceProvSetPaddingMapItem));
        context->properties->set_padding_map = lappend(context->properties->set_padding_map, setPaddingMapItem);
    }

    for (uint32 i = 0; i < metaHeader->num_set_graph_map_items; i++){
        TraceProvSetGraphMapItem *setGraphMapItem = tp_alloc0_object(TraceProvSetGraphMapItem);
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
        failSafeRead(file, &set_pointer_value, sizeof(int32_t));
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


void tp_parse_initialize_context(TraceProvParseContext *context){
    // The first layer is 1.
    context->global_layer_number = 1;
    context->unique_idx = 0;
    context->simple_incrementor = 0;
    context->properties = tp_alloc0_object(TraceProvParseGraphProperties);
    context->properties->set_padding_map = NIL;
    context->properties->set_graph_map = NIL;
    context->properties->sublink_map = NIL;
    context->properties->set_pointer_map = NIL;
    context->parent_targets = NIL;
}

void tp_add_set_pointer_property(TraceProvParseContext *context, const uint32 pointer, const uint32 ref){
    if (pointer == 0 || ref == 0)
        EXIT_WITH_MESSAGE("Invalid args!");

    TraceProvParseContext *real_context = GET_ROOT_CONTEXT(context);
    ListCell *cursor = NULL;
    foreach(cursor, real_context->properties->set_pointer_map){
        TraceProvSetPointerItem *item = (TraceProvSetPointerItem *)lfirst(cursor);
        if (item->set_pointer == pointer)
            break;
    }
    TraceProvSetPointerItem *item = NULL;
    if (cursor == NULL){
        item = tp_alloc0_object(TraceProvSetPointerItem);
        item->set_pointer = pointer;
        item->refs = NIL;
        real_context->properties->set_pointer_map = lappend(real_context->properties->set_pointer_map, item);
    }else{
        item = (TraceProvSetPointerItem *)lfirst(cursor);
    }
    item->refs = list_append_unique_int(item->refs, ref);
}

TraceProvDependency *_deserializeTraceProvDependency(FILE *file){
    TraceProvDependencyHeader header;
    failSafeRead(file, &header, sizeof(TraceProvDependencyHeader));
    if (TRACEPROV_GRAPH_IS_VALID(header.graphPtr)){
        TraceProvDependency *graph = tp_alloc0_object(TraceProvDependency);
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

static TraceProvEntry *_deserializeTraceProvEntry(FILE *input_file){
    TraceProvEntry *entry = tp_alloc0_object(TraceProvEntry);
    TraceProvEntryMetaHeader meta_header;
    failSafeRead(input_file, &meta_header, sizeof(TraceProvEntryMetaHeader));
    failSafeRead(input_file, entry, sizeof(TraceProvEntry));
    entry->sublinks = NIL;
    for (uint32 i = 0; i < meta_header.num_keys; i++){
        TraceProvTargetSublinkItem *sublink_item = tp_alloc0_object(TraceProvTargetSublinkItem);
        failSafeRead(input_file, sublink_item, sizeof(TraceProvTargetSublinkItem));
        entry->sublinks = lappend(entry->sublinks, sublink_item);
    }
    return entry;
}

TraceProvDependency *tp_get_sublink_graph(const TraceProvParseContext *parsed_context, TraceProvLayerNumber graph_number){
    ListCell *graph_cursor;
    foreach(graph_cursor, GET_ROOT_CONTEXT(parsed_context)->properties->sublink_map){
        TraceProvDependency *dependency = (TraceProvDependency *)lfirst(graph_cursor);
        if (dependency->headNumber == graph_number)
            return dependency;
    }
    EXIT_WITH_MESSAGE("Expected to always find the sublink!");
    return NULL;
}

}
// In some cases, we'd want to construct these dependencies progammatically.
void traceprov_mock_set_dependency(TraceProvParseContext *context, TraceProvDependency *dependency){
    mock_context = context;
    mock_dependency = dependency;
}
