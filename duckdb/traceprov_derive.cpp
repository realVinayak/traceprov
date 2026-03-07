#include <traceprov_node.hpp>
#include "traceprov.hpp"
#include "utils.hpp"
#include "derivation_utils.hpp"
#include "traceprov_settings.hpp"

extern "C" {
    #include "utils.h"
}

// The generic back derivation to SQL.
// Done here because some of the things are worth doing just for DuckDB.

static TraceProvInferAbstractTree* derive_on_node(
    TraceProvNode *node, 
    TraceProvDependency *graph,
    const uint32 idx_start,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    TraceProvRecursePack recurse_pack
);

static TraceProvInferAbstractTree *derive_set_on_node(
    TraceProvNode *reference_node,
    TraceProvDependency *curr_graph,
    const uint32 idx_start,
    const int set_number,
    const uint32 set_idx,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);
static TraceProvInferAbstractTree* derive_sublinks(
    TraceProvNode *node,
    const TraceProvDependency *graph,
    TraceProvParseContext *parse_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    // We cannot assume, indexes are 0-based
    // because we can be in a deep nested-agg chain.
    const uint32 idx_start,
    TraceProvRecursePack recurse_pack
);
TraceProvNode *simple_read_from_log(
    TraceProvDependency *graph,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context
);
static TraceProvInferAbstractTree *derive_aggregate_on_node(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);
static TraceProvInferAbstractTree *derive_aggregate_on_single_context(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);
static TraceProvInferAbstractTree *derive_aggregate_on_single_context_duckdb(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);
static TraceProvInferAbstractTree *derive_window_on_node(
    TraceProvNode *reference_node,
    const uint32 frame_start_idx,
    const uint32 frame_end_idx,
    // the _child_ graph.
    TraceProvDependency *curr_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);
static TraceProvInferAbstractTree *derive_window_on_single_context(
    TraceProvNode *reference_node,
    const uint32 frame_start_idx,
    const uint32 frame_end_idx,
    TraceProvDependency *curr_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
);

TraceProvPointerContext *traceprov_make_pointer_context();

static void pointer_context_add_layer(
    const TraceProvPointerContext *pointer_context,
    const TraceProvLayerNumber layer,
    uint32_t pointer_idx
);

void traceprov_infer_pointers(
    const TraceProvDependency *graph,
    const TraceProvPointerContext *pointer_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    const uint32_t start_idx = 0
);

std::vector<TraceProvWorkerLayer> *find_layers_across_workers(
    const TraceProvLayerNumber log_layer_number,
    const std::vector<struct local_context *> *worker_local_contexts,
    const uint32 expected_layer_width
){
    auto found_worker_layers = new std::vector<TraceProvWorkerLayer>;
    // Can have, at most, the number of workers.
    found_worker_layers->reserve(worker_local_contexts->size());
    for (auto worker_local_context: *worker_local_contexts){
        struct traceprov_aggregate_layer *candidate_layer = &worker_local_context->cached_layers[log_layer_number - 1];
        if (candidate_layer->layer_number == 0) continue;
        // If it is not 0, it should always be the current log layer...
        if (candidate_layer->layer_number != log_layer_number)
            elog(ERROR, "Got invalid log state!");

        if (expected_layer_width > 0 && (candidate_layer->num_pk_records != expected_layer_width))
            elog(ERROR, "Expeced the width to be consistent!");

        found_worker_layers->emplace_back(TraceProvWorkerLayer(worker_local_context->worker_id, candidate_layer));
    }

    if (found_worker_layers->size() == 0)
        elog(ERROR, "Expected the log to always be found!");

    PRINT_ON_VALIDATE("Found %ld log_layers for %d layer number", found_worker_layers->size(), log_layer_number);
    return found_worker_layers;
}

std::vector<TraceProvWorkerLayer> *find_combine_layers_across_workers(
    const TraceProvLayerNumber log_layer_number,
    const std::vector<struct local_context *> *worker_local_contexts
){
    auto found_worker_layers = new std::vector<TraceProvWorkerLayer>;
    found_worker_layers->reserve(worker_local_contexts->size());
    for (auto worker_local_context: *worker_local_contexts){
        for (uint32_t idx = 0; idx < TRACEPROV_MAX_LAYER_PER_WORKER; idx++){
            struct traceprov_aggregate_layer *candidate_layer = &worker_local_context->cached_layers[idx];
            if (candidate_layer->layer_number == 0) continue;
            if (candidate_layer->combined_aggregate_layer_number == log_layer_number){
                found_worker_layers->emplace_back(TraceProvWorkerLayer(worker_local_context->worker_id, candidate_layer));
                continue;
            }
        }
    }
    return found_worker_layers;
}

static std::vector<std::vector<uint64_t> *> *read_all_columns_simple(
    const struct traceprov_aggregate_layer *current_layer
){
    auto *current_worker_logs = new std::vector<std::vector<uint64_t> *>();
    current_worker_logs->reserve(current_layer->num_pk_records);
    for (uint32_t idx = 0; idx < current_layer->num_pk_records; idx++){
        auto data_vec = new std::vector<uint64_t>;
        current_worker_logs->push_back(data_vec);
    }
    return current_worker_logs;
}

static TraceProvData *read_all_columns(
    const TraceProvLayerNumber layer_number,
    const struct local_context *local_context,
    const TraceProvDependency *dependency
){
    const struct traceprov_aggregate_layer *current_layer = &local_context->cached_layers[layer_number - 1];
    if (current_layer->layer_number == 0) return nullptr;
    auto col_data = read_all_columns_simple(current_layer);
    // In this allocation scheme, all of them have rows layer.
    // So, this is not a useful addition.
    // if (current_layer->rows_layer_number){
    //     auto row_data = read_all_columns_simple(
    //         &local_context->cached_layers[current_layer->rows_layer_number - 1]
    //     );
    //     for (auto second: *row_data){
    //         col_data->push_back(second);
    //     }
    // }
    TraceProvData *return_data = new TraceProvData;
    for (auto column: *col_data){
        TraceProvColumnData *column_data = traceprov_make_empty_column();
        column_data->data = column;
        return_data->push_back(column_data);
    }
    return return_data;
}

static TraceProvInferAbstractTree *perform_derive_from_log_generic(
    TraceProvDependency *log_dependency,
    const uint8_t worker_count,
    TraceProvParseContext *parsed_back_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    const bool is_top_level,
    TraceProvRecursePack recurse_pack
){
    const TraceProvLayerNumber log_layer_number = log_dependency->headNumber;
    const uint32 layer_width = list_length(log_dependency->entries);
    auto found_worker_layers = find_layers_across_workers(
        log_layer_number,
        worker_local_contexts,
        layer_width
    );
    TraceProvInferAbstractTree *current_tree = makeTraceProvInferAbstractTree(log_dependency->headNumber);

    ListCell *entry_cursor = NULL;
    for (auto worker_log: *found_worker_layers){
        const uint8_t worker_id = worker_log.first;
        struct traceprov_aggregate_layer *agg_layer = worker_log.second;
        TraceProvRecursePack recurse_pack_worker = traceprov_shallow_copy_recurse_pack(&recurse_pack);
        TraceProvData *current_layer_data = read_all_columns(
            agg_layer->layer_number,
            worker_local_contexts->at(worker_id - 1),
            nullptr
        );
        TraceProvRelation *top_level_log_relation = make_traceprov_relation(current_layer_data, tp_psprintf("top_level_%s", tp_parse_get_unique_alias(parsed_back_context)));
        current_tree->children->push_back(
            derive_on_node(
                (TraceProvNode *)top_level_log_relation,
                log_dependency,
                0,
                worker_local_contexts->at(worker_id - 1),
                worker_local_contexts,
                parsed_back_context,
                recurse_pack_worker
            )
        );

        if (traceprov_use_implicit_union){
            // If doing implicit union, value of 0 implicitly implies that all workers need to be read.
            // Since we couldn't have 0 as the worker id before, this doesn't break anything from before :)
            top_level_log_relation->rel_args = make_relation_args(0, agg_layer->layer_number);
            break;          
        }else{
            top_level_log_relation->rel_args = make_relation_args(worker_id, agg_layer->layer_number);
        }

    }
    return current_tree;
}

TraceProvDerivationSpec *get_generic_derivation_spec(
    TraceProvParseContext **p_parsed_back_context,
    TraceProvInferSetupExtra **p_extra,
    char **p_parsed_query
){
    TraceProvParseContext *parsed_back_context = NULL;
    List *graphs = deserializeTraceProvDependency(&parsed_back_context, p_parsed_query, TRACEPROV_GRAPH_FILE);
    if (p_parsed_back_context)
        *p_parsed_back_context = parsed_back_context;
    struct traceprov_shared_context shared_context;
    if (map_traceprov_shared_context(&shared_context)){
        elog(ERROR, "Error mmaping shared context");
    }
    const uint8_t worker_count = shared_context.worker_count;
    ListCell *graph_cursor;

    auto worker_local_contexts = traceprov_get_local_contexts(worker_count);
    List *base_graph_depth_map = NIL;
    List *sublink_used_sublink_map = NIL;
    List *base_used_sublinks = traceprov_get_used_sublinks(graphs, &base_graph_depth_map, parsed_back_context);
    List *sublink_used_sublinks = traceprov_get_used_sublinks(parsed_back_context->properties->sublink_map, &sublink_used_sublink_map, parsed_back_context);
    List *get_all_used_sublinks = list_concat_copy(base_used_sublinks, sublink_used_sublinks);
    // traceprov_assert_equal_length(list_make2(base_graph_depth_map, graphs));
    auto top_tree = makeTraceProvInferAbstractTree(0);
    auto size_layer_map = new TraceProvSizeLayers;
    auto setup_extra = new TraceProvInferSetupExtra;
    setup_extra->size_layer_map = size_layer_map;
    setup_extra->worker_count = worker_count;
    if (p_extra)
        *p_extra = setup_extra;

    auto p_context = traceprov_make_pointer_context();
    foreach(graph_cursor, graphs){
        TraceProvDependency *graph = (TraceProvDependency *)lfirst(graph_cursor);
        // The top level graph should always be the simple log.
        if (graph->graph_type != TraceProvGraphKind::TP_LOG)
            elog(ERROR, "Expected the top level graph to always be a TP_LOG. Got %d", graph->graph_type);

        top_tree->children->push_back(
            perform_derive_from_log_generic(
                graph,
                worker_count,
                parsed_back_context,
                worker_local_contexts,
                true,
                TraceProvRecursePack {
                    .depth_map = (TraceProvDepthMap *)list_nth(base_graph_depth_map, foreach_current_index(graph_cursor)),
                    .level = 0,
                    .pending_sublinks = new TraceProvPendingSublinks,
                    .size_layer_map = size_layer_map
                }
            )
        );

        traceprov_infer_pointers(
            graph,
            p_context,
            worker_local_contexts
        );

    }
    ListCell *sublink_cursor;
    foreach(sublink_cursor, parsed_back_context->properties->sublink_map){
        TraceProvDependency *child_sublink = (TraceProvDependency*)lfirst(sublink_cursor);
        traceprov_infer_pointers(
            child_sublink,
            p_context,
            worker_local_contexts
        );
        // If a sublink is being used, don't derive it.
        // It should be automatically be derived as part of generic handling.
        if (list_member_int(get_all_used_sublinks, child_sublink->headNumber)) continue;
        if (child_sublink->graph_type != TraceProvGraphKind::TP_LOG)
            elog(ERROR, "Expected the top level graph to always be a TP_LOG. Got %d", child_sublink->graph_type);

        top_tree->children->push_back(
            perform_derive_from_log_generic(
                child_sublink,
                worker_count,
                parsed_back_context,
                worker_local_contexts,
                true,
                TraceProvRecursePack {
                    .depth_map = (TraceProvDepthMap *)list_nth(sublink_used_sublink_map, foreach_current_index(sublink_cursor)),
                    .level = 0,
                    .pending_sublinks = new TraceProvPendingSublinks,
                    .size_layer_map = size_layer_map
                }
            )
        );

    }

    auto result_map = new TraceProvResultMap;
    flattenTraceProvInferAbstractTree(top_tree, result_map, parsed_back_context);
    TraceProvDerivationSpec *spec = new TraceProvDerivationSpec;
    spec->p_context = p_context;
    spec->result_map = result_map;
    return spec;
}

// Generic handling of derivation.
// This doesn't care about whether node is a direct log
// This is used for nested aggregation + sublink handling.
static TraceProvInferAbstractTree* derive_on_node(
    TraceProvNode *node, 
    TraceProvDependency *graph,
    const uint32 idx_start,
    // This is a bit-finicky to get right.
    // There are cases where we _might_ be able to use the context, and restrict our search.
    // In most cases, this is expected to work. In cases it doesn't, we might just make repeated scans.
    // oh well. 
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    TraceProvRecursePack recurse_pack
){
    auto current_tree = makeTraceProvInferAbstractTree(graph->headNumber);
    ListCell *entry_cursor;
    bool did_append_self = false;
    foreach(entry_cursor, graph->entries){
        const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
        if (te->kind == TP_ENTRY_SET_POINTER){
            // The set pointer case.
            // Need to derive from this.
            const int current_set_number = te->setNumber;
            current_tree->children->push_back(
                derive_set_on_node(
                    node,
                    graph,
                    idx_start,
                    current_set_number,
                    foreach_current_index(entry_cursor),
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    recurse_pack
                )
            );
        }
        if (te->kind == TP_ENTRY_KIND_BASE_RELATION){
            if (!did_append_self){
                current_tree->nodes->push_back(node);
                // Also check if any sublink are refered in the entries.
                current_tree->children->push_back(
                    derive_sublinks(node, graph, parse_context, worker_local_contexts, idx_start, recurse_pack)
                );
                did_append_self = true;
            }
        } else if (te->kind == TP_ENTRY_KIND_POINTER){
            if (te->is_pointer_for_window) continue;
            TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(graph->children, foreach_current_index(entry_cursor));
            current_tree->children->push_back(
                derive_aggregate_on_node(
                    node,
                    idx_start + foreach_current_index(entry_cursor) + 1,
                    child_graph,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    traceprov_increment_recursion(&recurse_pack)
                )
            );
        } else if (te->kind == TP_ENTRY_FRAME_START){
            // The next entry should be the end.
            const TraceProvLayerNumber current_window_layer_number = te->window_entry.log_layer_number;
            const TraceProvEntry *next_te = (TraceProvEntry *)lfirst(lnext(graph->entries, entry_cursor));
            if (next_te->kind != TP_ENTRY_FRAME_END){
                elog(ERROR, "Expected the next entry to be the frame end!");
            }
            if (next_te->window_entry.log_layer_number != current_window_layer_number){
                elog(ERROR, "Got mismatching frame start and frame end log pointer!");
            }
            TraceProvDependency *child_dependency = tp_get_graph_from_children(graph, current_window_layer_number);
            current_tree->children->push_back(
                derive_window_on_node(
                    node,
                    idx_start + foreach_current_index(entry_cursor),
                    idx_start + foreach_current_index(entry_cursor) + 1,
                    child_dependency,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    recurse_pack
                )
            );
        }
    }
    return current_tree;
}

static TraceProvInferAbstractTree *derive_set_on_node(
    TraceProvNode *reference_node,
    TraceProvDependency *curr_graph,
    const uint32 idx_start,
    const int set_number,
    const uint32 set_idx,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    const int set_start_idx = traceprov_find_first_set_number_entry(curr_graph->entries, set_number);
    auto current_tree = makeTraceProvInferAbstractTree(curr_graph->headNumber);

    if (set_start_idx == -1)
        return current_tree;

    const List *possible_candidates = tp_get_set_pointer_property(parse_context, set_number);

    ListCell *candidate;
    foreach(candidate, possible_candidates){
        const int curr_set_number = lfirst_int(candidate);
        auto tp_filter = make_traceprov_filter(reference_node);
        tp_filter->const_join_condition->push_back(new TraceProvConstJoinPair(new TraceProvColumn(1, set_idx + idx_start + 1), (uint64_t)curr_set_number));
        
        current_tree->children->push_back(
            derive_on_node(
                (TraceProvNode *)tp_filter,
                tp_get_set_graph(parse_context, curr_set_number),
                idx_start + set_start_idx,
                current_local_context,
                worker_local_contexts,
                parse_context,
                recurse_pack
            )
        );
    }

    return current_tree;
}

static void pointer_context_add_layer(
    const TraceProvPointerContext *pointer_context,
    const TraceProvLayerNumber layer,
    uint32_t pointer_idx
){
    if (pointer_context->map->find(layer) == pointer_context->map->end()){
        pointer_context->map->insert({layer, new std::vector<uint32_t>});
    }else{
        elog(ERROR, "Didn't expect find an older copy!");
    }
    auto pointer_context_values = pointer_context->map->at(layer);
    pointer_context_values->push_back(pointer_idx);
}

TraceProvPointerContext *traceprov_make_pointer_context(){
    TraceProvPointerContext *p_context = new TraceProvPointerContext;
    p_context->map = new std::unordered_map<TraceProvLayerNumber, std::vector<uint32_t>*>;
    return p_context;
}

void traceprov_infer_pointers(
    const TraceProvDependency *graph,
    const TraceProvPointerContext *pointer_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    const uint32_t start_idx
){
    ListCell *entry_cursor;
    foreach(entry_cursor, graph->entries){
        const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
        if (te->kind == TP_ENTRY_KIND_POINTER){
            // Hmm not sure about this one.
            // TODO: Check if this stuff is valid for window too.
            if (te->is_pointer_for_window) continue;
            TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(graph->children, foreach_current_index(entry_cursor));
            auto worker_combine_layers = find_combine_layers_across_workers(
                child_graph->headNumber,
                worker_local_contexts
            );
            // Nothing to add.
            if (worker_combine_layers->size() == 0) continue;
            pointer_context_add_layer(
                pointer_context,
                graph->headNumber,
                start_idx + foreach_current_index(entry_cursor)
            );
            traceprov_infer_pointers(
                child_graph,
                pointer_context,
                worker_local_contexts,
                1
            );
        }
    }
}

static TraceProvInferAbstractTree* derive_sublinks(
    TraceProvNode *node,
    const TraceProvDependency *graph,
    TraceProvParseContext *parse_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    // We cannot assume, indexes are 0-based
    // because we can be in a deep nested-agg chain.
    const uint32 idx_start,
    TraceProvRecursePack recurse_pack
){
    const uint32 previous_start_idx = traceprov_get_node_column_count(node);
    auto current_tree = makeTraceProvInferAbstractTree(graph->headNumber);
    ListCell *entry_cursor;
    List *sublinks_to_commit = NIL;
    // Bitmapset *added_sublinks = NULL;
    foreach(entry_cursor, graph->entries){
        TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
        if (list_length(entry->sublinks) == 0) continue;
        ListCell *sublink_ref_cursor;
        foreach(sublink_ref_cursor, entry->sublinks){
            const TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_ref_cursor);
            uint32 max_level = recurse_pack.depth_map->at(item->layer_number);
            List *pending_entries = NIL;
            if (recurse_pack.pending_sublinks->find(item->layer_number) != recurse_pack.pending_sublinks->end()){
                pending_entries = recurse_pack.pending_sublinks->at(item->layer_number);
            }else{
                recurse_pack.pending_sublinks->insert({item->layer_number, NIL});
            }
            
            const uint32 insert_idx = item->offset_in_key;
            // All indexes are 1-indexed.
            uint32 entry_idx = idx_start + foreach_current_index(entry_cursor) + 1;

            pending_entries = traceprov_set_at_offset_int(pending_entries, insert_idx, entry_idx);

            recurse_pack.pending_sublinks->at(item->layer_number) = pending_entries;

            if (max_level == recurse_pack.level){
                // If it is the level when we're at the bottom,
                // need to "commit" and finally make the node entry.
                // The join conditions, with the sublink layer, will be everything that's the in the pending entries.
                // However, we can absolutely have multiple entries that point to the same sublink.
                // So need to delay the creation till we have seen all the entries. yuk.
                elog(INFO, "Stopping pending, for: %d", item->layer_number);
                sublinks_to_commit = list_append_unique_int(sublinks_to_commit, item->layer_number);
            }
        }
    }
    ListCell *pending_sublink_cursor;
    foreach(pending_sublink_cursor, sublinks_to_commit){
        // Need to actually derive sublinks now.
        TraceProvLayerNumber sublink_num = lfirst_int(pending_sublink_cursor);
        List *pending_entries = recurse_pack.pending_sublinks->at(sublink_num);
        elog(INFO, "Handling correlated of count: %d", list_length(pending_entries));
        auto join_condition = new TraceProvJoinConditions;
        ListCell *offset_cursor;
        foreach(offset_cursor, pending_entries){
            const int offset = lfirst_int(offset_cursor);
            const int other_idx = foreach_current_index(offset_cursor) + 1;
            TraceProvColumn *join_column_1 = new TraceProvColumn(1, offset);
            TraceProvColumn *join_column_2 = new TraceProvColumn(2, other_idx);
            join_condition->push_back(new TraceProvJoinPair(join_column_1, join_column_2));
        }
        TraceProvDependency *sublink_graph = tp_get_sublink_graph(parse_context, sublink_num);
        TraceProvNode *sublink_node = simple_read_from_log(sublink_graph, worker_local_contexts, parse_context);
        // need to read all the sublink logs, perform the join, and _then_ perform the derivation.
        // This is done because, otherwise, we may do a lot of work on the right hand side ultimately doing to waste
        // IF the optimizer is not able to reorder them...
        TraceProvJoinExpr *join_exprn = make_traceprov_join_expr(
            node,
            sublink_node,
            join_condition,
            nullptr,
            true,
            true
        );
        const uint32 new_idx_start = previous_start_idx;
        // Perform all the derivation on the join exprn now.
        current_tree->children->push_back(
            derive_on_node(
                (TraceProvNode *)join_exprn,
                sublink_graph,
                new_idx_start,
                (struct local_context *)NULL,
                worker_local_contexts,
                parse_context,
                recurse_pack
            )
        );
    }
    return current_tree;
}



TraceProvNode *simple_read_from_log(
    TraceProvDependency *graph,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context
){
    const TraceProvLayerNumber log_layer_number = graph->headNumber;
    const uint32 layer_width = list_length(graph->entries);
    auto found_worker_layers = find_layers_across_workers(
        log_layer_number,
        worker_local_contexts,
        layer_width
    );
    List *nodes = NIL;
    for (auto worker_log: *found_worker_layers){
        const uint8_t worker_id = worker_log.first;
        struct traceprov_aggregate_layer *agg_layer = worker_log.second;
        TraceProvData *current_layer_data = read_all_columns(
            agg_layer->layer_number,
            worker_local_contexts->at(worker_id - 1),
            nullptr
        );
        TraceProvRelation *relation = make_traceprov_relation(
            current_layer_data,
            tp_psprintf("log_read_to_append_%s",  tp_parse_get_unique_alias(parse_context))
        );
        nodes = lappend(nodes, relation);
        if (traceprov_use_implicit_union){
            // traceprov append consturctor is smart enough to simply return the first node
            // if the number of children is 1. So, this is absolutely fine. Nice.
            relation->rel_args = make_relation_args(0, agg_layer->layer_number);
            break;
        }else{
            relation->rel_args = make_relation_args(worker_id, agg_layer->layer_number);
        }
    }
    return make_traceprov_append(nodes, false);
}

static TraceProvInferAbstractTree *derive_aggregate_on_node(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    // const uint64 worker_count = worker_local_contexts->size();
    if ((agg_graph->graph_type != TraceProvGraphKind::TP_AGGREGATE) && (agg_graph->graph_type != TraceProvGraphKind::TP_PURE_AGGREGATE))
        elog(ERROR, "Expected the graph to always be of aggregate!");
    
    return derive_aggregate_on_single_context_duckdb(
            reference_node, 
            reference_match_idx, 
            agg_graph, 
            current_local_context, 
            worker_local_contexts, 
            parse_context, 
            recurse_pack
        );
}

static TraceProvInferAbstractTree *derive_aggregate_on_single_context_duckdb(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    auto current_tree = makeTraceProvInferAbstractTree(agg_graph->headNumber);
    const TraceProvLayerNumber layer_number_to_search = agg_graph->headNumber;
    
    auto layers_across_workers = find_layers_across_workers(agg_graph->headNumber, worker_local_contexts, 0);
    const uint64_t reference_node_col_count = traceprov_get_node_column_count(reference_node);

    // Now, need to iterate over the combined pairs.
    auto worker_combine_layers = find_combine_layers_across_workers(
        agg_graph->headNumber,
        worker_local_contexts
    );

    for (auto worker_layer_pair: *layers_across_workers){
        const auto worker_local_context = worker_local_contexts->at(worker_layer_pair.first - 1);
        auto curr_worker_logs = read_all_columns(layer_number_to_search, worker_local_context, nullptr);
        TraceProvJoinExpr *join_exprn = make_traceprov_simple_join(
            curr_worker_logs,
            (TraceProvNode *)reference_node,
            parse_context,
            "intermediate_join",
            reference_match_idx
        );

        join_exprn->is_left_star = true;

        uint64_t rel_flags = 0;
        if ((worker_combine_layers->size() == 0) || (traceprov_combine_in_memory)){
            current_tree->children->push_back(derive_on_node(
                (TraceProvNode*)join_exprn, 
                agg_graph, 
                reference_node_col_count,
                NULL,
                worker_local_contexts,
                parse_context,
                recurse_pack
            ));
        }

        if (traceprov_use_implicit_union){
            traceprov_get_relation_from_join(join_exprn)->rel_args = make_relation_args(0, layer_number_to_search);
            traceprov_get_relation_from_join(join_exprn)->rel_args->table_flags = rel_flags;
            break;
        }else{
            traceprov_get_relation_from_join(join_exprn)->rel_args = make_relation_args(worker_layer_pair.first, layer_number_to_search);
        }
    }

    // Ugh, TODO: Refactor. 
    if (!traceprov_combine_in_memory){
        for (auto worker_layer_pair: *worker_combine_layers){
            // For each combine, need to, unfortunately, join will all the previous ones.
            // No pruning (yet) :/
            const auto combiner_worker_local_context = worker_local_contexts->at(worker_layer_pair.first - 1);
            auto combine_logs = read_all_columns(worker_layer_pair.second->layer_number, combiner_worker_local_context, nullptr);
            TraceProvRelation *combine_relation = make_traceprov_relation(
                combine_logs,
                tp_psprintf("combined_entry")
            );

            TraceProvColumn *output_column_1 = new TraceProvColumn(2, 2); // This is the individual log
            TraceProvColumn *output_column_2 = new TraceProvColumn(2, 3); // This is the worker id
            auto output_column = new std::vector<TraceProvColumn*>;
            output_column->push_back(output_column_1);

            if (!traceprov_use_implicit_union)
                output_column->push_back(output_column_2);

            TraceProvColumn *join_column_1 = new TraceProvColumn(1, reference_match_idx);
            TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
            auto join_condition = new TraceProvJoinConditions;
            join_condition->push_back(new std::pair<TraceProvColumn*, TraceProvColumn*>(join_column_1, join_column_2));
            auto combine_join = make_traceprov_join_expr(
                reference_node,
                (TraceProvNode *)combine_relation,
                join_condition,
                output_column,
                true
            );

            const uint64_t combine_match_key_idx = traceprov_get_node_column_count((TraceProvNode *)combine_join);
            if (combine_match_key_idx != (traceprov_get_node_column_count(reference_node) + 2))
                elog(INFO, "Inconsistent state!");
            const uint64_t worker_id_key_idx = combine_match_key_idx;
        
            for (auto remote_worker_local_pair: *layers_across_workers){
                const uint8_t worker_id = remote_worker_local_pair.first;
                auto base_logs = read_all_columns(agg_graph->headNumber, worker_local_contexts->at(worker_id - 1), nullptr);
                auto base_log_relation = make_traceprov_relation(base_logs, tp_psprintf("base_join_%s", tp_parse_get_unique_alias(parse_context)));
                TraceProvJoinExpr *base_join_exprn = make_traceprov_join_from_rel(
                    reference_node, 
                    (TraceProvNode *)base_log_relation,
                    base_log_relation->data->size(),
                    reference_match_idx
                );
                base_join_exprn->is_left_star = true;
                TraceProvJoinExpr *partial_join_exprn = make_traceprov_join_from_rel(
                    (TraceProvNode *)combine_join,
                    (TraceProvNode *)base_log_relation,
                    base_log_relation->data->size(),
                    combine_match_key_idx
                );

                for (uint col_idx = 0; col_idx < reference_node_col_count; col_idx++){
                    partial_join_exprn->output_columns->insert(
                        partial_join_exprn->output_columns->begin() + col_idx,
                        new TraceProvColumn(1, col_idx + 1)
                        );
                }

                if (!traceprov_use_implicit_union){
                    TraceProvColumn *worker_id_column = new TraceProvColumn(1, worker_id_key_idx);
                    partial_join_exprn->const_join_condition->push_back(new TraceProvConstJoinPair(worker_id_column, worker_id));
                }
                if (traceprov_get_node_column_count((TraceProvNode *)partial_join_exprn) != traceprov_get_node_column_count((TraceProvNode *)base_join_exprn)){
                    elog(ERROR, "Got mismatching node count on logs!");
                }

                current_tree->children->push_back(derive_on_node(
                    (TraceProvNode*)partial_join_exprn, 
                    agg_graph, 
                    reference_node_col_count,
                    NULL,
                    worker_local_contexts,
                    parse_context,
                    traceprov_shallow_copy_recurse_pack(&recurse_pack)
                ));

                if (traceprov_use_implicit_union){
                    base_log_relation->rel_args = make_relation_args(0, agg_graph->headNumber);
                    break;
                }else{
                    base_log_relation->rel_args = make_relation_args(worker_id, agg_graph->headNumber);
                }
            }
            if (traceprov_use_implicit_union){
                // When doing implicit union, we need to return all combines.
                // However, since combine layers are arbitrarily numbered, it is possible that the same layer number
                // does not mean the same across workers. So, need this.
                combine_relation->rel_args = make_relation_args(0, agg_graph->headNumber);
                combine_relation->rel_args->table_flags |= TRACEPROV_TABLE_COMBINE;
                break;
            }else{
                combine_relation->rel_args = make_relation_args(combiner_worker_local_context->worker_id, worker_layer_pair.second->layer_number);
            }
        }
    }


    return current_tree;
}


static TraceProvInferAbstractTree *derive_aggregate_on_single_context(
    TraceProvNode *reference_node,
    const uint32 reference_match_idx,
    TraceProvDependency *agg_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    auto current_tree = makeTraceProvInferAbstractTree(agg_graph->headNumber);
    const TraceProvLayerNumber layer_number_to_search = agg_graph->headNumber;
    const struct traceprov_aggregate_layer *layer = &current_local_context->cached_layers[layer_number_to_search - 1];
    if (layer->layer_number == 0)
        return current_tree;

    auto self_logs = read_all_columns(layer_number_to_search, current_local_context, nullptr);
    const uint64_t reference_node_col_count = traceprov_get_node_column_count(reference_node);
    const bool aggregate_was_split = layer->is_leader_layer;
    // While we don't need to actually split the data (we can't do that anyways)
    // we still need to read data to make base relations out of it.
    if (!aggregate_was_split){
        // The simple case.
        if (agg_graph->graph_type == TP_PURE_AGGREGATE && (recurse_pack.pending_sublinks->size() == 0)){

            auto base_log_relation = make_traceprov_relation(self_logs, tp_psprintf("base_join_%s", tp_parse_get_unique_alias(parse_context)));
            base_log_relation->rel_args = make_relation_args(current_local_context->worker_id, agg_graph->headNumber);
            auto child_tree = derive_on_node(
                (TraceProvNode *)base_log_relation,
                agg_graph,
                0,
                current_local_context,
                worker_local_contexts,
                parse_context,
                recurse_pack
            );
            List *derived_nodes = getTraceProvInferAbstractTreeNodes(child_tree);
            ListCell *cursor;
            foreach(cursor, derived_nodes){
                TraceProvNode **node = (TraceProvNode **)lfirst(cursor);
                auto exists_node = make_traceprov_exists(*node, reference_node);
                *node = (TraceProvNode *)exists_node;
            }
            current_tree->children->push_back(child_tree);
        }else{
            TraceProvJoinExpr *join_exprn = make_traceprov_simple_join(
                self_logs,
                (TraceProvNode *)reference_node,
                parse_context,
                "intermediate_join",
                reference_match_idx
            );

            traceprov_get_relation_from_join(join_exprn)->rel_args = make_relation_args(current_local_context->worker_id, layer_number_to_search);

            join_exprn->is_left_star = true;
            current_tree->children->push_back(derive_on_node(
                (TraceProvNode*)join_exprn, 
                agg_graph, 
                reference_node_col_count,
                current_local_context,
                worker_local_contexts,
                parse_context,
                recurse_pack
            ));
        }
    }else{
        // This is a slightly complicated case.
        auto layers_across_workers = find_layers_across_workers(agg_graph->headNumber, worker_local_contexts, 0);
        TraceProvWorkerLayer leader_layer_pair;
        bool found_leader = false;
        for (auto worker_layer_pair: *layers_across_workers){
            if (worker_layer_pair.second->is_leader_layer){
                leader_layer_pair = worker_layer_pair;
                found_leader = true;
                break;
            }
        }
        if (!found_leader)
            elog(ERROR, "In the split case, always expected to find the leader layer!");

        auto leader_worker_context = worker_local_contexts->at(leader_layer_pair.first - 1);
        TraceProvLayerNumber combine_layer_number = leader_layer_pair.second->combined_aggregate_layer_number;
        auto combine_logs = read_all_columns(combine_layer_number, leader_worker_context, nullptr);
        TraceProvRelation *combine_relation = make_traceprov_relation(
            combine_logs,
            tp_psprintf("combined_entry")
        );
        combine_relation->rel_args = make_relation_args(leader_worker_context->worker_id, combine_layer_number);
        // Need to:
        // 1. Join the combine logs to the previous log.
        // 2. Join the combine logs (in an union) to all the local worker logs.
        // 3. Join the base logs directly to the previous log.

        // For combine, need to join on colum 1, but need to propagate column 2 and 3
        TraceProvColumn *output_column_1 = new TraceProvColumn(2, 2); // This is the worker id
        TraceProvColumn *output_column_2 = new TraceProvColumn(2, 3); // This is the individual log 
        auto output_column = new std::vector<TraceProvColumn*>;
        output_column->push_back(output_column_1);
        output_column->push_back(output_column_2);

        TraceProvColumn *join_column_1 = new TraceProvColumn(1, reference_match_idx);
        TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
        auto join_condition = new TraceProvJoinConditions;
        join_condition->push_back(new std::pair<TraceProvColumn*, TraceProvColumn*>(join_column_1, join_column_2));
        auto combine_join = make_traceprov_join_expr(
            reference_node,
            (TraceProvNode *)combine_relation,
            join_condition,
            output_column,
            true
        );
        const uint64_t combine_match_key_idx = traceprov_get_node_column_count((TraceProvNode *)combine_join);
        if (combine_match_key_idx != (traceprov_get_node_column_count(reference_node) + 2))
            elog(INFO, "Inconsistent state!");
        const uint64_t worker_id_key_idx = combine_match_key_idx - 1;

        for (auto worker_local_pair: *layers_across_workers){
            const uint8_t worker_id = worker_local_pair.first;
            auto base_logs = read_all_columns(agg_graph->headNumber, worker_local_contexts->at(worker_id - 1), nullptr);
            // TODO: Reinvestigate why this condition was needed...
            if (base_logs == nullptr) continue;
            auto base_log_relation = make_traceprov_relation(base_logs, tp_psprintf("base_join_%s", tp_parse_get_unique_alias(parse_context)));
            base_log_relation->rel_args = make_relation_args(worker_id, agg_graph->headNumber);
            TraceProvJoinExpr *base_join_exprn = make_traceprov_join_from_rel(
                reference_node, 
                (TraceProvNode *)base_log_relation,
                base_log_relation->data->size(),
                reference_match_idx
            );
            base_join_exprn->is_left_star = true;
            TraceProvJoinExpr *partial_join_exprn = make_traceprov_join_from_rel(
                (TraceProvNode *)combine_join,
                (TraceProvNode *)base_log_relation,
                base_log_relation->data->size(),
                combine_match_key_idx
            );
            // We cannot output everything from the left star (need to only output upto the join columns)
            for (uint col_idx = 0; col_idx < reference_node_col_count; col_idx++){
                partial_join_exprn->output_columns->insert(
                    partial_join_exprn->output_columns->begin() + col_idx,
                    new TraceProvColumn(1, col_idx + 1)
                    );
            }
            TraceProvColumn *worker_id_column = new TraceProvColumn(1, worker_id_key_idx);
            partial_join_exprn->const_join_condition->push_back(new TraceProvConstJoinPair(worker_id_column, worker_id));
            if (traceprov_get_node_column_count((TraceProvNode *)partial_join_exprn) != traceprov_get_node_column_count((TraceProvNode *)base_join_exprn)){
                elog(ERROR, "Got mismatching node count on logs!");
            }

            if (agg_graph->graph_type == TP_PURE_AGGREGATE && (recurse_pack.pending_sublinks->size() == 0)){
                auto child_tree = derive_on_node(
                    (TraceProvNode *)base_log_relation,
                    agg_graph,
                    0,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    traceprov_shallow_copy_recurse_pack(&recurse_pack)
                );
                List *derived_nodes = getTraceProvInferAbstractTreeNodes(child_tree);
                ListCell *cursor;
                foreach(cursor, derived_nodes){
                    TraceProvNode **node = (TraceProvNode **)lfirst(cursor);
                    // It should be very abornormal to be in a case where we'd get finalized in a child worker.
                    // This is because, even in the case where there's a partition, we're still acting on all the data.
                    auto exists_node = make_traceprov_exists(*node, (TraceProvNode*)partial_join_exprn);
                    *node = (TraceProvNode *)exists_node;
                }
                current_tree->children->push_back(child_tree);
            }else{
                current_tree->children->push_back(derive_on_node(
                    (TraceProvNode*)partial_join_exprn, 
                    agg_graph, 
                    reference_node_col_count,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    traceprov_shallow_copy_recurse_pack(&recurse_pack)
                ));

                current_tree->children->push_back(derive_on_node(
                    (TraceProvNode*)base_join_exprn, 
                    agg_graph, 
                    reference_node_col_count,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    traceprov_shallow_copy_recurse_pack(&recurse_pack)
                ));
            }
        }
    }

    return current_tree;
}

static TraceProvInferAbstractTree *derive_window_on_node(
    TraceProvNode *reference_node,
    const uint32 frame_start_idx,
    const uint32 frame_end_idx,
    // the _child_ graph.
    TraceProvDependency *curr_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    if (current_local_context != NULL){
        return derive_window_on_single_context(
            reference_node,
            frame_start_idx,
            frame_end_idx,
            curr_graph,
            current_local_context,
            worker_local_contexts,
            parse_context,
            recurse_pack
        );
    }

    auto current_tree = makeTraceProvInferAbstractTree(curr_graph->headNumber);
    for (auto context: *worker_local_contexts){
        current_tree->children->push_back(
            derive_window_on_single_context(
                reference_node,
                frame_start_idx,
                frame_end_idx,
                curr_graph,
                context,
                worker_local_contexts,
                parse_context,
                traceprov_shallow_copy_recurse_pack(&recurse_pack)
            )
        );
    }
    return current_tree;
}

static TraceProvInferAbstractTree *derive_window_on_single_context(
    TraceProvNode *reference_node,
    const uint32 frame_start_idx,
    const uint32 frame_end_idx,
    TraceProvDependency *curr_graph,
    const struct local_context *current_local_context,
    const std::vector<struct local_context *> *worker_local_contexts,
    TraceProvParseContext *parse_context,
    const TraceProvRecursePack recurse_pack
){
    auto current_tree = makeTraceProvInferAbstractTree(curr_graph->headNumber);
    const TraceProvLayerNumber layer_number_to_search = curr_graph->headNumber;
    const struct traceprov_aggregate_layer *layer = &current_local_context->cached_layers[layer_number_to_search - 1];
    if (layer->layer_number == 0)
        return current_tree;

    // Everything from the left side + logged entries from the right side.
    const uint64_t reference_node_col_count = traceprov_get_node_column_count(reference_node);
    const uint64_t log_col_count = list_length(curr_graph->entries);
    const uint64_t column_count = reference_node_col_count + log_col_count;
    TraceProvWindowRead *window_read = make_traceprov_window_read(
        frame_start_idx,
        frame_end_idx,
        column_count,
        reference_node
    );
    window_read->rel_args = make_relation_args(current_local_context->worker_id, layer_number_to_search);
    current_tree->children->push_back(
        derive_on_node(
            (TraceProvNode *)window_read,
            curr_graph,
            reference_node_col_count,
            current_local_context,
            worker_local_contexts,
            parse_context,
            // Because we're going another level down...
            traceprov_increment_recursion(&recurse_pack)
        )
    );
    if (recurse_pack.size_layer_map->find(log_col_count) == recurse_pack.size_layer_map->end()){
        recurse_pack.size_layer_map->insert({log_col_count, new std::vector<TraceProvLayerNumber>});   
    }
    auto layer_vector = recurse_pack.size_layer_map->at(log_col_count);
    layer_vector->push_back(layer_number_to_search);
    return current_tree;
}