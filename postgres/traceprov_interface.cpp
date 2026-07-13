// Some logic that is useful to have as part of the c file, but need cpp logic.

#include <unordered_map>
#include <vector>

extern "C" {
    #include "traceprov_interface.h"
    #include "traceprov_graph.h"
    #include "traceprov_parse_context.h"
    


    // Instead of using Postgres List, here, heap is used.
    // This is because we may actually not be in a long lived context when this gets called :/
    typedef std::unordered_map<uint32_t, std::vector<uint32_t>* > TraceProvStatsCollectorMap;

    // Needed because need to differnetiate between cases where
    // state is invalid and null.
    #define TP_INVALID_STATS_STATE ((TraceProvStatsCollectorMap *)(-1))

    typedef struct InterfaceInternal {
        TraceProvStatsCollectorMap *stats_collector_map;
    } InterfaceInternal;

    static InterfaceInternal g_context {
        .stats_collector_map = nullptr
    };

    void traceprov_reset_interface(){
        if (g_context.stats_collector_map && g_context.stats_collector_map != TP_INVALID_STATS_STATE){
            for (auto entry: *g_context.stats_collector_map){
                delete entry.second;
            }
            delete g_context.stats_collector_map;
        }
        g_context.stats_collector_map = nullptr;
    }

    static inline void _add_stats_vector(const uint32_t layer, TraceProvStatsCollectorMap *stats_collector_map){
        if (stats_collector_map->find(layer) == stats_collector_map->end()){
            stats_collector_map->insert({layer, new std::vector<uint32_t>});
        }
    }

    void populate_stat_columns(const TraceProvDependency *dependency, TraceProvStatsCollectorMap *stats_collector_map){
        ListCell *entry_cursor;
        const auto layer_num = (uint32_t)dependency->headNumber;
        if (dependency->graph_type != TP_LOG){
            _add_stats_vector(layer_num, stats_collector_map);
            stats_collector_map->at(layer_num)->push_back(0);
        }
        foreach(entry_cursor, dependency->entries){
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            const bool needs_stats_kind = (
                te->kind == TP_ENTRY_KIND_POINTER
                || te->kind == TP_ENTRY_CORRELATION_ATTR
                || te->kind == TP_ENTRY_IN_CORRELATION_ATTR
            );
            const bool needs_stats_sublinks = list_length(te->sublinks) > 0;
            const int curr_idx = foreach_current_index(entry_cursor);
            if (needs_stats_kind  || needs_stats_sublinks){
                _add_stats_vector(layer_num, stats_collector_map);
                stats_collector_map->at(layer_num)->push_back(curr_idx + (dependency->graph_type == TP_LOG ? 0 : 1));
            }
            if (te->kind == TP_ENTRY_KIND_POINTER){
                populate_stat_columns((TraceProvDependency *)list_nth(dependency->children, curr_idx), stats_collector_map);
            }
        }
    }
    // Figures out which columns should have 
    TraceProvStatsCollectorMap *traceprov_get_stat_columns(){
        auto stats_collector = new TraceProvStatsCollectorMap;
        TraceProvParseContext *parsed_back_context = NULL;
        List *graphs = deserializeTraceProvDependency(&parsed_back_context, NULL);
        if (graphs == NIL){
            return NULL;
        }
        ListCell *graph_cursor;
        foreach(graph_cursor, graphs){
            TraceProvDependency *graph = (TraceProvDependency *)lfirst(graph_cursor);
            populate_stat_columns(graph, stats_collector);
        }
        foreach(graph_cursor, parsed_back_context->properties->sublink_map){
            TraceProvDependency *child_sublink = (TraceProvDependency *)lfirst(graph_cursor);
            populate_stat_columns(child_sublink, stats_collector);
        }
        return stats_collector;
    }

    static inline void setup_interface(){
        if (g_context.stats_collector_map == nullptr){
            g_context.stats_collector_map = traceprov_get_stat_columns();
            if (g_context.stats_collector_map == NULL){
                g_context.stats_collector_map = TP_INVALID_STATS_STATE;
            }
        }
    }

    List *traceprov_get_null_columns(const uint32_t query_layer){
        setup_interface();
        if (g_context.stats_collector_map == TP_INVALID_STATS_STATE) return NIL;
        if (g_context.stats_collector_map == nullptr) return NIL;
        if (g_context.stats_collector_map->find(query_layer) == g_context.stats_collector_map->end()) return NIL;
        auto columns =  g_context.stats_collector_map->at(query_layer);
        List *l_columns = NIL;
        for(auto col: *columns){
            l_columns = lappend_int(l_columns, col);
        }
        return l_columns;
    }
}
