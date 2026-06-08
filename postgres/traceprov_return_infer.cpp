// Like traceprov's normal infer, but returns the set of all rows inline for postgres
// rather than dumping logic.

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>
#include <fstream>
#include "traceprov_infer_essentials.hpp"
#include "traceprov_ext_utils.hpp"
#include "traceprov_node.hpp"
#include "traceprov_infer.hpp"

#undef HAVE__BUILTIN_TYPES_COMPATIBLE_P

extern "C"
{
#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "traceprov.h"
#include "file_utils.h"
#include "utils/builtins.h"
#include "traceprov_parse_context.h"
#include "rewriter_utils.h"

    PG_MODULE_MAGIC;
}

extern "C"
{
    // static_assert(!(HAVE__BUILTIN_TYPES_COMPATIBLE_P))

#define UNUSED(X) \
    do            \
    {             \
    } while (0 && X);

    uint64 traceprov_get_node_column_count(TraceProvNode *node);
    // Converts the node to SQL.
    static char *traceprov_node_to_sql(TraceProvNode *node, TraceProvToSQLContext context);

    static TraceProvData *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const std::vector<struct local_context *> *worker_local_context,
        const TraceProvDependency *dependency,
        bool emulate_read = false);

    static TraceProvColumnData *traceprov_make_empty_column();
    static void traceprov_materialize_derived_result(FunctionCallInfo fcinfo, TraceProvTopResult *derived);

    static TraceProvNode *simple_read_from_log(
        TraceProvDependency *graph,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack);

    static TraceProvInferAbstractTree *derive_on_node(
        TraceProvNode *node,
        TraceProvDependency *graph,
        const uint32 idx_start,
        const struct local_context *current_local_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack);

    static TraceProvInferAbstractTree *makeTraceProvInferAbstractTree(const TraceProvLayerNumber layer_number)
    {
        TraceProvInferAbstractTree *tree = palloc0_object(TraceProvInferAbstractTree);
        tree->layer_number = layer_number;
        tree->children = new std::vector<TraceProvInferAbstractTree *>;
        tree->nodes = new std::vector<TraceProvNode *>;
        return tree;
    }

    // Get all children that lie in a tree.
    // We could return a vector (instead of pg list), but that's awkward to deal with it.
    static List *getTraceProvInferAbstractTreeNodes(const TraceProvInferAbstractTree *tree)
    {
        List *nodes = NIL;
        for (uint64 idx = 0; idx < tree->nodes->size(); idx++)
        {
            // This is a pointer to the node, because we'll be mutating that in-place :)
            nodes = lappend(nodes, &tree->nodes->at(idx));
        }
        for (auto child : *tree->children)
        {
            nodes = list_concat(nodes, getTraceProvInferAbstractTreeNodes(child));
        }
        return nodes;
    }

    static void flattenTraceProvInferAbstractTree(TraceProvInferAbstractTree *tree, TraceProvResultMap *result_map, TraceProvParseContext *parse_context);

    static TraceProvDerivedNode *traceprov_make_derived_node(TraceProvLayerNumber layer_number, TraceProvNode *node)
    {
        TraceProvDerivedNode *derived = (TraceProvDerivedNode *)malloc(sizeof(TraceProvDerivedNode));
        derived->layer_number = layer_number;
        derived->node = node;
        return derived;
    }

    static TraceProvData *traceprov_make_column_data(
        std::vector<std::vector<uint64_t> *> *vectors)
    {
        auto data = palloc0_object(TraceProvData);
        for (auto column : *vectors)
        {
            auto column_repr = traceprov_make_empty_column();
            column_repr->data = column;
            data->push_back(column_repr);
        }
        return data;
    }

    static TraceProvColumnData *traceprov_make_empty_column()
    {
        auto col_data = palloc0_object(TraceProvColumnData);
        col_data->data = new std::vector<uint64_t>;
        return col_data;
    }

    static TraceProvRelation *get_relation_from_join(TraceProvJoinExpr *join_exprn, bool right = true)
    {
        TraceProvNode *node = right ? join_exprn->right : join_exprn->left;
        if (node->tag != T_TP_RELATION)
            elog(ERROR, "Expected to always be a relation!");
        TraceProvRelation *relation = (TraceProvRelation *)node;
        return relation;
    }

    std::vector<TraceProvWorkerLayer> *find_layers_across_workers(
        TraceProvLayerNumber log_layer_number,
        const std::vector<struct local_context *> *worker_local_contexts,
        const uint32 expected_layer_width,
        const bool do_strict = true);

    static int find_first_set_number_entry(const List *entries, int set_number);

    struct infer_result
    {
        size_t width;
        std::vector<uint64> **ids;
    };

    void store_inference(
        Tuplestorestate *tupstore,
        TupleDesc tupdesc,
        const struct infer_result *result)
    {
        const size_t width = result->width;
        std::vector<uint64> **pk_records = result->ids;
        bool *nulls = (bool *)malloc(sizeof(bool) * width);
        memset(nulls, 0, sizeof(bool) * width);

        Datum *records = (Datum *)malloc(sizeof(Datum) * width);

        for (size_t record_index = 0; record_index < pk_records[0]->size(); record_index++)
        {
            for (size_t key_index = 0; key_index < width; key_index++)
            {
                records[key_index] = Int64GetDatum(pk_records[key_index]->at(record_index));
            }

            tuplestore_putvalues(tupstore, tupdesc, records, nulls);
        }

#if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
#endif
        return;
    }

    std::vector<uint64> *set_diff(std::vector<uint64> *first, std::vector<uint64> *second)
    {
        // set diff, assumes sorted.
        unsigned long int iter_first = 0;
        unsigned long int iter_second = 0;
        std::vector<uint64> *set_diff_computed = new std::vector<uint64>;
        while (iter_first < first->size())
        {
            bool did_loop = false;
            while ((iter_second < second->size()) && (first->at(iter_first) == second->at(iter_second)))
            {
                did_loop = true;
                iter_second++;
            }
            if (did_loop)
            {
                iter_first++;
                continue;
            }
            set_diff_computed->push_back(first->at(iter_first++));
        }
        return set_diff_computed;
    }

    // Takes the computation graph, and performs inference using it.
    // TODO: The tagging is needed in the case where aggregate is performed (finalized) in parallel worker
    // In that case, the result will, incorrectly, be a pointer (need to fix that.)
    static void perform_inference_graph_worker(
        struct traceprov_shared_context *sharedContext,
        const TraceProvDependency *graph,
        std::vector<uint64> *reference,
        uint8 worker_id)
    {
        // We need to get the recorded input in the layer file (given the reference)
        // It is possible that we used parallel (omitted for now.)
        const TraceProvLayerNumber layerNumber = graph->headNumber;
        const struct local_context *context = NULL;
        const struct traceprov_aggregate_layer *layer = &context->cached_layers[graph->headNumber - 1];

        std::sort(reference->begin(), reference->end());

        void *ptr_layer_row = NULL;
        if (map_layer_file(layerNumber, worker_id, &ptr_layer_row, layer->size))
        {
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        const void *ptr_final_row = get_final_ptr(ptr_layer_row, layer);
        const int entryCount = list_length(graph->entries);

        std::vector<std::vector<uint64> *> *entriesValues = new std::vector<std::vector<uint64> *>;
        for (int i = 0; i < entryCount; i++)
        {
            entriesValues->push_back(new std::vector<uint64>);
        }

        if (layer->num_pk_records != (uint32)list_length(graph->entries))
        {
            elog(ERROR, "expected to log the count of entries");
        }

        while (ptr_layer_row < ptr_final_row)
        {
            ptr_layer_row = (void *)(((uint64)layer->record_padding) + (uint64)ptr_layer_row);
            bool found = false;
            found = std::binary_search(reference->begin(), reference->end(), ((struct trace_file_forward_row *)ptr_layer_row)->group_count);
            if (found)
            {
                for (uint32 key_idx = 0; key_idx < layer->num_pk_records; key_idx++)
                {
                    const int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row *)ptr_layer_row), key_idx);
                    entriesValues->at(key_idx)->push_back(record_key);
                }
            }

            ptr_layer_row = (void *)GET_PK_FROM_ROW(((struct trace_file_forward_row *)ptr_layer_row), layer->num_pk_records);
        }

        for (int i = 0, pointerChildIdx = 0; i < entryCount; i++)
        {
            elog(INFO, "Computed lengths: %ld", entriesValues->at(i)->size());
            const TraceProvEntry *tpEntry = (TraceProvEntry *)list_nth(graph->entries, i);
            // If it is a pointer, also need to keep recursing down to the children.
            if (tpEntry->kind == TraceProvEntryKind::TP_ENTRY_KIND_POINTER)
            {
                perform_inference_graph_worker(
                    sharedContext,
                    (TraceProvDependency *)list_nth(graph->children, pointerChildIdx),
                    entriesValues->at(i),
                    worker_id);
                pointerChildIdx++;
            }
        }
    }

    void perform_inference_driver()
    {
        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping shared context");
        }
        TraceProvParseContext *parseContext;

        const TraceProvDependency *graph = (TraceProvDependency *)lfirst(list_head(deserializeTraceProvDependency(&parseContext, NULL)));
        const int group_layer_number = graph->headNumber + 1;
        void *group_layer_ptr = NULL;
        const struct local_context *main_worker_context = NULL;
        const struct traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[graph->headNumber - 1];
        const struct traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];

        std::vector<uint64> *top_level_values = new std::vector<uint64>;
        if (map_layer_file(group_layer_number, context.main_worker_id, &group_layer_ptr, group_layer->size))
        {
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        for (uint64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++)
        {
            const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
            if (gr->in_result)
            {
                top_level_values->push_back(group_idx + 1);
            }
        }

        perform_inference_graph_worker(&context, graph, top_level_values, 0);
    }

    struct infer_result *perform_inference(
        const unsigned int layer_number,
        const unsigned int reference_layer,
        const unsigned int subq_layer_number)
    {

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping shared context");
        }

        const int group_layer_number = layer_number + 1;
        const int partial_group_ln = layer_number + 2;

        const struct local_context *main_worker_context = NULL;
        const struct traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[layer_number - 1];
        const struct traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];
        const struct traceprov_aggregate_layer *partial_group_layer = &main_worker_context->cached_layers[partial_group_ln - 1];

        auto present_groups = new std::vector<uint64>;
        if (subq_layer_number)
        {
            std::vector<uint64> **subq_records = NULL;
            int subq_width = 0;
            // Here, it is entirely possible that the subquery gets parallelized.
            // So, we'd have to look at all the workers.
            const int subq_index = subq_layer_number - 1;
            for (int worker_id = 0; worker_id < context.worker_count; worker_id++)
            {

                const struct local_context *bg_context = NULL;
                const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[subq_index];

                if (bg_trace_layer->layer_number != subq_layer_number)
                    continue;

                subq_width = bg_trace_layer->num_pk_records + 1;
                if (subq_records == NULL)
                {
                    // Need to have +1 because of the adjusting that was done during tracing.
                    subq_records = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64> *) * (bg_trace_layer->num_pk_records + 1));
                    for (uint32 pk_id = 0; pk_id < bg_trace_layer->num_pk_records + 1; pk_id++)
                        subq_records[pk_id] = new std::vector<uint64>;
                }

                void *subq_forward_row = NULL;
                if (map_layer_file(subq_layer_number, worker_id, &subq_forward_row, bg_trace_layer->size))
                {
                    PRINT_ON_DEBUG("Error opening the subq trace file");
                    continue;
                }

                const void *subq_final_row = get_final_ptr(subq_forward_row, bg_trace_layer);

                while (subq_forward_row < subq_final_row)
                {
                    subq_forward_row = (void *)((uint64)bg_trace_layer->record_padding + (uint64)subq_forward_row);
                    int64 *subq_forward_row_record = (int64 *)subq_forward_row;
                    for (uint32 key_idx = 0; key_idx < bg_trace_layer->num_pk_records + 1; key_idx++, subq_forward_row_record++)
                    {
                        subq_records[key_idx]->push_back(*subq_forward_row_record);
                    }
                    subq_forward_row = (void *)subq_forward_row_record;
                }
            }

            struct infer_result *infer_result_computed = (struct infer_result *)malloc(sizeof(struct infer_result));

            infer_result_computed->ids = subq_records;
            infer_result_computed->width = subq_width;
            return infer_result_computed;
        }

        // Need to mmap the group layer file now. We're going to go mmap the entire file (because we can't have data
        // past the file contents).

        void *group_layer_ptr;
        void *forward_row;
        void *partial_group_row = NULL;

        if (map_layer_file(group_layer_number, context.main_worker_id, &group_layer_ptr, group_layer->size))
        {
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        std::vector<uint64> *groups_to_filter = new std::vector<uint64>;

        if (reference_layer)
        {
            const struct traceprov_aggregate_layer *group_reference_layer = &main_worker_context->cached_layers[reference_layer];
            void *group_reference_ptr = NULL;
            if (map_layer_file(reference_layer + 1, context.main_worker_id, &group_reference_ptr, group_reference_layer->size))
            {
                PRINT_ON_DEBUG("Error opening group reference layer");
                elog(ERROR, "Error opening group reference layer");
            }
            for (uint64 reference_group_idx = 0; reference_group_idx < group_layer->num_groups; reference_group_idx++)
            {
                const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_reference_ptr)[reference_group_idx];
                if (gr->in_result)
                {
                    groups_to_filter->push_back(reference_group_idx + 1);
                }
            }

            std::sort(groups_to_filter->begin(), groups_to_filter->end());
        }

        for (uint64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++)
        {
            const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
            int should_add = false;
            if (reference_layer)
            {
                should_add = std::binary_search(groups_to_filter->begin(), groups_to_filter->end(), gr->in_result);
            }
            else
            {
                should_add = gr->in_result;
            }
            if (should_add)
            {
                present_groups->push_back(group_idx + 1);
            }
        }

        if (map_layer_file(layer_number, context.main_worker_id, &forward_row, main_trace_layer->size))
        {
            PRINT_ON_DEBUG("Error opening main trace file");
            elog(ERROR, "Error opening main trace file");
        }

        if (0 == access(
                     get_bi_injected_str(TRACEPROV_MAIN_TRACE_FILE, DataDir, partial_group_ln, main_worker_context->worker_id, NULL),
                     F_OK))
        {
            if (map_layer_file(partial_group_ln, context.main_worker_id, &partial_group_row, partial_group_layer->size))
            {
                PRINT_ON_DEBUG("Error opening the partial trace file");
            }
        }

        if (partial_group_row)
            PRINT_ON_DEBUG("Using partial trace file");

        std::sort(present_groups->begin(), present_groups->end());

        std::vector<uint64> **groups_per_worker = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64> *) * (context.worker_count));
        for (int worker_id = 0; worker_id < context.worker_count; worker_id++)
            groups_per_worker[worker_id] = new std::vector<uint64>;

        std::vector<uint64> *present_groups_found = new std::vector<uint64>;

        if (partial_group_row != NULL)
        {
            // Need to, now, find the rows in the partial file.
            const void *final_partial_group_row_ptr = get_final_ptr(partial_group_row, partial_group_layer);

            while (partial_group_row < final_partial_group_row_ptr)
            {
                partial_group_row = (void *)((uint64)partial_group_layer->record_padding + (uint64)partial_group_row);
                const struct trace_file_partial_row *current_partial_row = (struct trace_file_partial_row *)partial_group_row;
                // Essentially, if the global group number gets found, store the local group number.
                if (std::binary_search(present_groups->begin(), present_groups->end(), current_partial_row->global_group_number))
                {
                    groups_per_worker[current_partial_row->worker_id]->push_back(current_partial_row->local_group_number);
                    present_groups_found->push_back(current_partial_row->global_group_number);
                }

                partial_group_row = (void *)((uint8 *)partial_group_row + sizeof(struct trace_file_partial_row));
            }
        }

        if (partial_group_row == NULL)
            groups_per_worker[context.main_worker_id] = present_groups;

        std::vector<uint64> **filtered_rows = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64> *) * (main_trace_layer->num_pk_records));

        for (uint32 key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++)
            filtered_rows[key_idx] = new std::vector<uint64>;

        int iters_made = 0;
        std::vector<uint64> *main_worker_set_difference = nullptr;

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++)
        {

            std::vector<uint64> *local_group_nos = groups_per_worker[worker_id];
            // In case of main worker, we can be in the case where the group was completely within our portion of the table
            // In that case, we'd miss logging it in the local_group_nos.
            if (local_group_nos->size() == 0 && worker_id != context.main_worker_id)
                continue;

            std::sort(local_group_nos->begin(), local_group_nos->end());

            const struct local_context *bg_context = NULL;
            const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[layer_number - 1];

            void *current_forward_row = NULL;
            if (worker_id == context.main_worker_id)
            {
                current_forward_row = forward_row;
            }
            else
            {
                if (map_layer_file(layer_number, worker_id, &current_forward_row, bg_trace_layer->size))
                {
                    PRINT_ON_DEBUG("Error opening the bg trace file");
                }
            }

            const void *current_final_row = get_final_ptr(current_forward_row, bg_trace_layer);

            while (current_forward_row < current_final_row)
            {
                current_forward_row = (void *)((uint64)bg_trace_layer->record_padding + (uint64)current_forward_row);

                bool found = false;
                found = std::binary_search(local_group_nos->begin(), local_group_nos->end(), ((struct trace_file_forward_row *)current_forward_row)->group_count);
                if (!found && worker_id == context.main_worker_id)
                {
                    // Now, we'd need to compute the set difference. It is deferred till here.
                    if (main_worker_set_difference == nullptr)
                    {
                        std::sort(present_groups_found->begin(), present_groups_found->end());
                        main_worker_set_difference = set_diff(present_groups, present_groups_found);
                    }
                    found = std::binary_search(main_worker_set_difference->begin(), main_worker_set_difference->end(), ((struct trace_file_forward_row *)current_forward_row)->group_count);
                }

                if (found)
                {
                    for (uint32 key_idx = 0; key_idx < bg_trace_layer->num_pk_records; key_idx++)
                    {
                        int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row *)current_forward_row), key_idx);

                        filtered_rows[key_idx]->push_back(record_key);
                    }
                }
                iters_made++;
                current_forward_row = (void *)GET_PK_FROM_ROW(((struct trace_file_forward_row *)current_forward_row), bg_trace_layer->num_pk_records);
            }
        }

        struct infer_result *infer_result_computed = (struct infer_result *)malloc(sizeof(struct infer_result));
        infer_result_computed->ids = filtered_rows;
        infer_result_computed->width = main_trace_layer->num_pk_records;

        return infer_result_computed;
    }

    PG_FUNCTION_INFO_V1(traceprov_infer);

    Datum traceprov_infer(FunctionCallInfo fcinfo)
    {

        ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
        TupleDesc tupdesc;
        Tuplestorestate *tupstore;
        MemoryContext per_query_ctx;
        MemoryContext oldcontext;

        const int32 layer_number = PG_GETARG_INT32(0);
        const int32 reference_layer = PG_GETARG_INT32(1);
        const int32 subq_layer_number = PG_GETARG_INT32(2);

        if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("set-valued function called in context that cannot accept a set")));
        if (!(rsinfo->allowedModes & SFRM_Materialize))
            ereport(ERROR,
                    (errcode(ERRCODE_SYNTAX_ERROR),
                     errmsg("materialize mode required, but it is not allowed in this context")));

        /* Switch into long-lived context to construct returned data structures */
        per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
        oldcontext = MemoryContextSwitchTo(per_query_ctx);

        if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
            elog(ERROR, "return type must be a row type");

        tupstore = tuplestore_begin_heap(true, false, work_mem);
        rsinfo->returnMode = SFRM_Materialize;
        rsinfo->setResult = tupstore;
        rsinfo->setDesc = tupdesc;

        MemoryContextSwitchTo(oldcontext);

        struct infer_result *infer_result_computed = perform_inference(layer_number, reference_layer, subq_layer_number);
        store_inference(tupstore, tupdesc, infer_result_computed);

        return (Datum)0;
    }

    // Performs inference via the graph.
    PG_FUNCTION_INFO_V1(traceprov_infer_graph);

    Datum traceprov_infer_graph(PG_FUNCTION_ARGS)
    {
        perform_inference_driver();
        PG_RETURN_INT64(0);
    }

    PG_FUNCTION_INFO_V1(traceprov_infer_time);

    // Runs the inference, and returns just the time taken to complete the inference.
    // Note that it is the time to just fill-up the buffer with primary keys.
    // So, it is a good indicator of overhead of inference (rather than materialization)
    Datum traceprov_infer_time(FunctionCallInfo fcinfo)
    {

        const int32 layer_number = PG_GETARG_INT32(0);
        const int32 reference_layer = PG_GETARG_INT32(1);
        const int32 subq_layer_number = PG_GETARG_INT32(2);

        auto start = std::chrono::steady_clock::now();

        struct infer_result *infer_result_computed = perform_inference(layer_number, reference_layer, subq_layer_number);

        auto end = std::chrono::steady_clock::now();

        UNUSED(infer_result_computed);

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        uint64 duration_time = (uint64)duration.count();

        PG_RETURN_INT64(duration_time);
    }

    PG_FUNCTION_INFO_V1(traceprov_sync_time);

    Datum traceprov_sync_time(FunctionCallInfo fcinfo)
    {
        std::vector<std::string> *messages = new std::vector<std::string>;

        TP_EVALUATE_START();

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping the shared context for sync!");
        }

        int final_code = 0;
        const auto worker_local_contexts = traceprov_get_local_contexts(context.worker_count);

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++)
        {

            struct local_context *worker_local_context = worker_local_contexts->at(worker_id);

            for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++)
            {

                if (worker_local_context->cached_layers[layer_id].layer_number)
                {
                    void *ptr = NULL;
                    if (map_layer_file(
                            worker_local_context->cached_layers[layer_id].layer_number,
                            worker_id + 1,
                            &ptr,
                            worker_local_context->cached_layers[layer_id].size))
                    {
                        elog(ERROR, "Error opening layer file!");
                    }
                    char worker_layer[128] = {0};
                    if (ptr == NULL)
                    {
                        messages->push_back("Skipping");
                        continue;
                    }
                    sprintf(worker_layer, "(WORKER: %d, Layer: %d)", worker_id + 1, layer_id);
                    messages->push_back(worker_layer);
                    final_code |= (msync(ptr, worker_local_context->cached_layers[layer_id].size * TRACEPROV_PAGE_SIZE, MS_SYNC));
                    if (final_code)
                    {
                        elog(ERROR, "Error doing the msync!");
                    }
                }
            }
        }

        TP_EVALUATE_END();

        const uint64_t duration = TP_EVALUATE_DURATION();

        for (std::string s : *messages)
        {
            elog(INFO, "SYNC: %s", s.c_str());
        }
        elog(INFO, "Final code: %d", final_code);
        PG_RETURN_INT64(duration);
    }

    // Prints some useful statistics (like # of pks, # of groups)
    PG_FUNCTION_INFO_V1(traceprov_layer_stat);

    Datum traceprov_layer_stat(FunctionCallInfo fcinfo)
    {

        // Making it privatrre be there are ton of places that refer to these variables
        // and compile binds them tot this enum.
        enum TRACEPROV_LAYER_STAT
        {
            is_leader_layer,
            worker_id,
            layer_id,
            num_pk_records,
            layer_size,
            num_groups,
            layer_number,
            record_padding,
            layer_fd,
            logged_record_count,
            is_sorted_by_group_num,
            aggregate_strategy,
            buckets,
            combined_aggregate_layer_number,
            rows_layer_number,
            null_map_layer_number,
            null_map,
            last_allocation_size,
            initial_allocation_size,

            NUM_COLUMNS
        };

        ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
        TupleDesc tupdesc;
        Tuplestorestate *tupstore;
        MemoryContext per_query_ctx;
        MemoryContext oldcontext;

        if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("set-valued function called in context that cannot accept a set")));
        if (!(rsinfo->allowedModes & SFRM_Materialize))
            ereport(ERROR,
                    (errcode(ERRCODE_SYNTAX_ERROR),
                     errmsg("materialize mode required, but it is not allowed in this context")));

        per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
        oldcontext = MemoryContextSwitchTo(per_query_ctx);

        if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
            elog(ERROR, "return type must be a row type");

        tupstore = tuplestore_begin_heap(true, false, work_mem);
        rsinfo->returnMode = SFRM_Materialize;
        rsinfo->setResult = tupstore;
        rsinfo->setDesc = tupdesc;

        MemoryContextSwitchTo(oldcontext);

        struct traceprov_shared_context context;

        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping the shared context");
        }

        Datum record[TRACEPROV_LAYER_STAT::NUM_COLUMNS];
        bool nulls[TRACEPROV_LAYER_STAT::NUM_COLUMNS];
        memset(nulls, 0, sizeof(bool) * TRACEPROV_LAYER_STAT::NUM_COLUMNS);

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++)
        {

            int fd = open(psprintf(TRACEPROV_WORKER_LAYER_MAP, DataDir, worker_id + 1), O_RDONLY);
            if (fd < 0)
                elog(ERROR, "Error opening the worker laye rmap!");
            void *ptr = mmap(
                NULL,
                sizeof(struct local_context),
                PROT_READ,
                MAP_SHARED,
                fd,
                0);
            if (ptr == MAP_FAILED)
            {
                elog(ERROR, "Error mmaping the layer file!");
            }
            struct local_context *worker_local_context = (struct local_context *)ptr;
            std::vector<struct local_context *> local_context_vec = {worker_local_context};

            for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++)
            {

                struct traceprov_aggregate_layer layer = worker_local_context->cached_layers[layer_id];

                if (layer.layer_number == 0)
                    continue;

                const uint32 record_size = layer.record_padding + (layer.num_pk_records) * sizeof(int64);

                const uint64 final_ptr_offset = (uint64)get_final_ptr(NULL, &layer);

                if (record_size != 0)
                {
                    if (final_ptr_offset % record_size)
                    {
                        elog(INFO, "Expected ptr offset to be multiple of record size at (WORKER: %d, LAYER: %d)!", worker_id, layer_id);
                    }
                }

                int64 record_count = 0;
                int32 is_sorted_by_group_no = -1;

                record_count = layer.size == 0 ? 0 : read_all_columns(layer.layer_number, &local_context_vec, nullptr)->at(0)->data->size();
                record[TRACEPROV_LAYER_STAT::is_leader_layer] = Int32GetDatum(layer.is_leader_layer);
                record[TRACEPROV_LAYER_STAT::worker_id] = Int32GetDatum(worker_id + 1);
                record[TRACEPROV_LAYER_STAT::layer_id] = Int32GetDatum(layer.layer_number);
                record[TRACEPROV_LAYER_STAT::num_pk_records] = Int32GetDatum(layer.num_pk_records);
                record[TRACEPROV_LAYER_STAT::layer_size] = Int32GetDatum(layer.size);
                record[TRACEPROV_LAYER_STAT::num_groups] = Int32GetDatum(layer.num_groups);
                record[TRACEPROV_LAYER_STAT::layer_number] = Int32GetDatum(layer.layer_number);
                record[TRACEPROV_LAYER_STAT::record_padding] = Int32GetDatum(layer.record_padding);
                record[TRACEPROV_LAYER_STAT::layer_fd] = Int32GetDatum(layer.layer_fd);
                record[TRACEPROV_LAYER_STAT::logged_record_count] = Int64GetDatum(record_count);
                record[TRACEPROV_LAYER_STAT::is_sorted_by_group_num] = Int32GetDatum(is_sorted_by_group_no);
                record[TRACEPROV_LAYER_STAT::aggregate_strategy] = Int32GetDatum(layer.aggregate_strategy);
                std::string graphStr = "[";
                for (int i = 0; i < TRACEPROV_BUCKET_COUNT - 1; i++)
                {
                    graphStr = graphStr.append(psprintf("%d", layer.buckets[i]));
                }
                graphStr.append("]");
                record[TRACEPROV_LAYER_STAT::buckets] = PointerGetDatum(cstring_to_text(graphStr.c_str()));
                record[TRACEPROV_LAYER_STAT::combined_aggregate_layer_number] = Int32GetDatum(layer.combined_aggregate_layer_number);
                record[TRACEPROV_LAYER_STAT::rows_layer_number] = Int32GetDatum(layer.rows_layer_number);
                record[TRACEPROV_LAYER_STAT::null_map_layer_number] = Int32GetDatum(layer.null_map_layer_number);
                record[TRACEPROV_LAYER_STAT::null_map] = Int64GetDatum(layer.null_map);
                record[TRACEPROV_LAYER_STAT::last_allocation_size] = Int64GetDatum(layer.last_allocation_size);
                record[TRACEPROV_LAYER_STAT::initial_allocation_size] = Int64GetDatum(layer.initial_allocation_size);
                tuplestore_putvalues(tupstore, tupdesc, record, nulls);
            }
            traceprov_fail_safe_unmap(ptr, sizeof(struct local_context));
        }
#if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
#endif
        return (Datum)0;
    }

    // Performs inference via the graph.
    PG_FUNCTION_INFO_V1(traceprov_json_graph);

    Datum traceprov_json_graph(PG_FUNCTION_ARGS)
    {
        TraceProvParseContext *context = NULL;
        const List *graphs = deserializeTraceProvDependency(&context, NULL);
        ListCell *graph_cursor;
        StringInfoData buf;
        initStringInfo(&buf);
        std::string graph_str = "{";
        graph_str = graph_str.append("\"graphs\": [");
        bool needsSep = false;
        foreach (graph_cursor, graphs)
        {
            if (needsSep)
            {
                graph_str = graph_str.append(",");
            }
            needsSep = true;
            const TraceProvDependency *graph = (TraceProvDependency *)lfirst(graph_cursor);
            const char *graphRepr = traceProvDependencyToJson(graph);
            elog(INFO, "%s", graphRepr);
            graph_str = graph_str.append(graphRepr);
        }
        // End "graphs" key.
        graph_str = graph_str.append("]");
        graph_str = graph_str.append(",");
        graph_str = graph_str.append("\"context\":");
        const char *contextRepr = traceProvParseContextToJson(context);
        elog(INFO, "%s", contextRepr);
        graph_str = graph_str.append(contextRepr);
        graph_str = graph_str.append("}");
        elog(INFO, "%s", graph_str.c_str());
        PG_RETURN_TEXT_P(cstring_to_text(graph_str.c_str()));
    }

    PG_FUNCTION_INFO_V1(traceprov_parsed_back);

    Datum traceprov_parsed_back(PG_FUNCTION_ARGS)
    {
        TraceProvParseContext *context = NULL;
        char *final_parsed_back = NULL;
        deserializeTraceProvDependency(&context, &final_parsed_back);
        PG_RETURN_TEXT_P(cstring_to_text(final_parsed_back));
    }

    TraceProvRelationArgs *make_relation_args(uint64_t worker_id, uint64_t layer_number)
    {
        auto rel_args = palloc0_object(TraceProvRelationArgs);
        rel_args->worker_id = worker_id;
        rel_args->layer_number = layer_number;
        return rel_args;
    }

    TraceProvRelation *make_traceprov_relation(TraceProvData *data, char *name)
    {
        if (data->size() == 0)
            elog(ERROR, "Expected to have at least 1 column!");
        auto tp_rel = palloc0_object(TraceProvRelation);
        tp_rel->tag = T_TP_RELATION;
        tp_rel->data = data;
        tp_rel->name = name;
        tp_rel->rel_args = NULL;
        PRINT_ON_VALIDATE("REL: %s, %ld;", name, data->at(0)->data->size());
        return tp_rel;
    }

    TraceProvWindowRead *make_traceprov_window_read(
        const uint64 frame_start_idx,
        const uint64 frame_end_idx,
        const uint64 column_count,
        TraceProvNode *child_node)
    {
        auto tp_window_read = palloc0_object(TraceProvWindowRead);
        tp_window_read->tag = T_TP_WINDOW_READ;
        tp_window_read->frame_start_idx = frame_start_idx;
        tp_window_read->frame_end_idx = frame_end_idx;
        tp_window_read->column_count = column_count;
        tp_window_read->child_node = child_node;
        return tp_window_read;
    }

    TraceProvFilter *make_traceprov_filter(
        TraceProvNode *child_node)
    {
        auto tp_filter_node = palloc0_object(TraceProvFilter);
        tp_filter_node->tag = T_TP_FILTER;
        tp_filter_node->const_join_condition = new TraceProvConstJoinPairs;
        tp_filter_node->child_node = child_node;
        return tp_filter_node;
    }

    TraceProvExists *make_traceprov_exists(
        TraceProvNode *current,
        TraceProvNode *condition)
    {
        auto tp_exists_node = palloc0_object(TraceProvExists);
        tp_exists_node->tag = T_TP_EXISTS;
        tp_exists_node->current = current;
        tp_exists_node->condition = condition;
        return tp_exists_node;
    }

    static void traceprov_assert_is_in_range(const uint64 column_count, TraceProvColumn *column)
    {
        if (column->second > column_count)
            elog(ERROR, "Table access out of range!");
    }

    TraceProvJoinExpr *make_traceprov_join_expr(
        TraceProvNode *left,
        TraceProvNode *right,
        TraceProvJoinConditions *join_condition,
        std::vector<TraceProvColumn *> *output_columns,
        bool is_left_star = false,
        bool is_right_star = false,
        bool is_single_result = false)
    {
        auto tp_join_exprn = palloc0_object(TraceProvJoinExpr);
        tp_join_exprn->tag = T_TP_JOIN;
        tp_join_exprn->left = left;
        tp_join_exprn->right = right;
        const auto left_column_count = traceprov_get_node_column_count(left);
        const auto right_column_count = traceprov_get_node_column_count(right);
        for (auto condition : *join_condition)
        {
            if (condition->first->first != 1 || condition->second->first != 2)
                elog(ERROR, "Numberig is inconsistent!");
            traceprov_assert_is_in_range(left_column_count, condition->first);
            traceprov_assert_is_in_range(right_column_count, condition->second);
            if (condition->first->second == 0 || condition->second->second == 0)
            {
                elog(ERROR, "Got column 0 as the condition!");
            }
        }
        if (output_columns)
        {
            for (auto output_column_value : *output_columns)
            {
                if (output_column_value->second == 0)
                {
                    elog(ERROR, "Got invalid value 0 as the column index!");
                }
            }
        }
        tp_join_exprn->join_condition = join_condition;
        tp_join_exprn->output_columns = output_columns;
        tp_join_exprn->result = nullptr;
        tp_join_exprn->is_left_star = is_left_star;
        tp_join_exprn->is_right_star = is_right_star;
        tp_join_exprn->is_single_result = is_single_result;
        tp_join_exprn->const_join_condition = new TraceProvConstJoinPairs;
        return tp_join_exprn;
    }

    // We may not actually end up returning an append, depending on how things went.
    // So, the return type is a node (rather than append node)
    TraceProvNode *make_traceprov_append(
        List *nodes,
        bool is_lazy)
    {
        if (list_length(nodes) == 0)
            elog(ERROR, "Making append rel with no nodes!");

        if (list_length(nodes) == 1)
        {
            return (TraceProvNode *)list_nth(nodes, 0);
        }

        auto tp_append = palloc0_object(TraceProvAppend);
        tp_append->tag = T_TP_APPEND;

        ListCell *node_cursor;
        uint64 column_count = 0;
        foreach (node_cursor, nodes)
        {
            if (column_count == 0)
            {
                column_count = traceprov_get_node_column_count((TraceProvNode *)lfirst(node_cursor));
                continue;
            }
            if (column_count != traceprov_get_node_column_count((TraceProvNode *)lfirst(node_cursor)))
                elog(ERROR, "Got differing column count in append!");
        }
        tp_append->nodes = nodes;
        tp_append->is_lazy = is_lazy;
        return (TraceProvNode *)tp_append;
    }

    TraceProvJoinExpr *make_traceprov_join_from_rel(
        TraceProvNode *left_side,
        TraceProvNode *right_side,
        int output_column_side,
        uint64 join_idx_offset)
    {

        TraceProvColumn *join_column_1 = new TraceProvColumn(1, join_idx_offset);
        TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
        auto join_conditions = new TraceProvJoinConditions;
        join_conditions->push_back(new std::pair<TraceProvColumn *, TraceProvColumn *>(join_column_1, join_column_2));
        auto output_columns = new std::vector<TraceProvColumn *>(output_column_side - 1);
        for (int i = 0; i < output_column_side - 1; i++)
        {
            output_columns->at(i) = (new TraceProvColumn(2, i + 2));
        }

        return make_traceprov_join_expr(
            left_side,
            right_side,
            join_conditions,
            output_columns);
    }

    TraceProvJoinExpr *make_traceprov_simple_join(
        TraceProvData *self_logs,
        TraceProvNode *join_side,
        TraceProvParseContext *parse_context,
        const char *rel_name,
        uint64 join_side_idx = 1)
    {

        if (self_logs == nullptr)
            elog(ERROR, "Expected self logs to always be set!");

        auto right_side = (TraceProvNode *)make_traceprov_relation(self_logs, psprintf("%s_%s", rel_name, tp_parse_get_unique_alias(parse_context)));

        return make_traceprov_join_from_rel(join_side, right_side, self_logs->size(), join_side_idx);
    }

    bool is_nested_agg(TraceProvDependency *graph)
    {
        ListCell *entry_cursor;
        bool is_pointer = false;
        foreach (entry_cursor, graph->entries)
        {
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->kind == TP_ENTRY_KIND_POINTER)
            {
                if (is_pointer)
                    elog(ERROR, "Found multiple pointers, not supported, for now.");
                is_pointer = true;
            }
        }
        if (is_pointer && (list_length(graph->entries) != 1) && (list_length(graph->children) != 1))
            elog(ERROR, "In the pointer case, expected the length of graph entries to be 1, for now.");
        return is_pointer;
    }

    static TraceProvNode *expand_child_aggregate(
        TraceProvDependency *parent_graph_head,
        TraceProvNode *match_side,
        struct local_context *local_context,
        TraceProvParseContext *parse_context)
    {
        if (match_side->tag != T_TP_JOIN)
            elog(ERROR, "Expected the match side to always be a join, for now.");

        auto child_graph = (TraceProvDependency *)list_nth(parent_graph_head->children, 0);
        TraceProvJoinExpr *join_side = (TraceProvJoinExpr *)match_side;
        // No longer necessary.
        // if (join_side->output_columns->size() != 1)
        //     elog(ERROR, "Currently, only handling single output columns in nesting..");

        const TraceProvLayerNumber agg_layer_number = child_graph->headNumber;
        std::vector<struct local_context *> local_context_vector = {local_context};
        auto layer_data = read_all_columns(agg_layer_number, &local_context_vector, child_graph);
        TraceProvJoinExpr *join_exprn = make_traceprov_simple_join(
            layer_data,
            (TraceProvNode *)join_side,
            parse_context,
            "intermediate_join",
            traceprov_get_node_column_count(match_side));
        // Prepend to include all, other than the join column.
        for (uint64 col_idx = 1; col_idx < traceprov_get_node_column_count(match_side); col_idx++)
        {
            join_exprn->output_columns->insert(join_exprn->output_columns->begin(), new TraceProvColumn(1, col_idx));
        }
        if (is_nested_agg(child_graph))
        {
            return expand_child_aggregate(
                child_graph,
                (TraceProvNode *)join_exprn,
                local_context,
                parse_context);
        }
        return (TraceProvNode *)join_exprn;
    }

    // Reads all the columns
    // Doesn't go into rows.
    static std::vector<std::vector<uint64_t> *> *read_all_columns_simple(
        const struct traceprov_aggregate_layer *current_layer,
        const struct local_context *local_context,
        bool emulate_read)
    {
        const uint8 worker_id = local_context->worker_id;
        void *log_ptr = NULL;
        if (map_layer_file(current_layer->layer_number, worker_id, &log_ptr, current_layer->size))
            elog(ERROR, "Error mapping (worker: %d, layer: %d)", worker_id, current_layer->layer_number);

        const void *final_log_ptr = get_final_ptr(log_ptr, current_layer);
        const uint32 layer_record_padding = current_layer->record_padding;
        auto current_worker_logs = new std::vector<std::vector<uint64_t> *>();
        current_worker_logs->reserve(current_layer->num_pk_records);
        const uint64 total_number_of_records = (current_layer->size * TRACEPROV_PAGE_SIZE) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        // Reserve space for next pointers.
        for (uint32 i = 0; i < current_layer->num_pk_records; i++)
        {
            auto data_vec = new std::vector<uint64_t>;
            // If not emulating read, only then reserve the space, because we ain't filling it up.
            if (!emulate_read)
                data_vec->reserve(total_number_of_records);
            current_worker_logs->push_back(data_vec);
        }

        if (emulate_read)
            return current_worker_logs;

        while (log_ptr < final_log_ptr)
        {
            log_ptr = (void *)((char *)log_ptr + layer_record_padding);
            uint64 *log_canonical_ptr = (uint64 *)log_ptr;
            for (uint32 entry_id = 0; entry_id < current_layer->num_pk_records; entry_id++, log_canonical_ptr++)
            {
                current_worker_logs->at(entry_id)->push_back(*log_canonical_ptr);
            }
            log_ptr = log_canonical_ptr;
        }
        return current_worker_logs;
    }

    // Reads all columns. Also looks into rows.
    // If emulate read, it doesn't perform the actural read, but creates column vectors.
    static TraceProvData *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const std::vector<struct local_context *> *worker_local_contexts,
        const TraceProvDependency *dependency,
        bool emulate_read)
    {
        // const auto evaluate_start = std::chrono::steady_clock::now();
        struct local_context *local_context = NULL;
        if (worker_local_contexts->size() == 1)
        {
            local_context = worker_local_contexts->at(0);
        }
        else
        {
            auto found_layers = find_layers_across_workers(layer_number, worker_local_contexts, 0);
            // Doesn't really matter which layer we end up choosing.
            // This is done just so the query construction and column count is accurate.
            local_context = worker_local_contexts->at(found_layers->at(0).first - 1);
        }
        const struct traceprov_aggregate_layer *current_layer = &local_context->cached_layers[layer_number - 1];
        // In this case, the layer wasn't set.
        if (current_layer->layer_number != layer_number)
        {
            elog(ERROR, "Expected log to be set: %d:%d!", current_layer->layer_number, layer_number);
        }
        std::vector<TraceProvOffset *> *data = read_all_columns_simple(current_layer, local_context, emulate_read);

        if (current_layer->rows_layer_number)
        {
            auto row_data = read_all_columns_simple(
                &local_context->cached_layers[current_layer->rows_layer_number - 1],
                local_context,
                emulate_read);
            for (auto second : *row_data)
            {
                data->push_back(second);
            }
        }
        PRINT_ON_VALIDATE("READ LAYER: %d, got: %ld", layer_number, data->at(0)->size());
        TraceProvData *return_data = new TraceProvData;
        for (auto column : *data)
        {
            TraceProvColumnData *column_data = traceprov_make_empty_column();
            column_data->data = column;
            return_data->push_back(column_data);
        }
        dependency = nullptr;

        if (dependency != nullptr)
        {
            if (((uint64)list_length(dependency->entries)) != data->size())
                elog(ERROR, "Expected lengths to be the same!");
            ListCell *entry_cursor = 0;
            foreach (entry_cursor, dependency->entries)
            {
                TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
                TraceProvDescriptor *descriptor = palloc0_object(TraceProvDescriptor);
                descriptor->layer_number = layer_number;
                descriptor->entry = entry;
                return_data->at(foreach_current_index(entry_cursor))->descriptor = descriptor;
            }
        }
        // const auto evaluate_end = std::chrono::steady_clock::now();
        // PRINT_ON_VALIDATE("read all took: %ld", std::chrono::duration_cast<std::chrono::microseconds>(evaluate_end - evaluate_start).count());
        return return_data;
    }

    char *get_sample_values(TraceProvData *data)
    {
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfo(&buf, "[SAMPLE OF %ld]: sample_values: {", data->at(0)->data->size());
        for (uint64 i = 0; i < Min(data->at(0)->data->size(), 10); i++)
        {
            appendStringInfo(&buf, "[");
            for (uint64 col = 0; col < data->size(); col++)
            {
                if (col > 0)
                    appendStringInfo(&buf, ",");
                appendStringInfo(&buf, "%ld", data->at(col)->data->at(i));
            }
            appendStringInfo(&buf, "]");
        }
        appendStringInfo(&buf, "}");
        return buf.data;
    }

    char *traceprov_node_to_string(const TraceProvNode *node)
    {
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '(');
        if (node->tag == T_TP_RELATION)
        {
            const TraceProvRelation *relation = (TraceProvRelation *)node;
            appendStringInfo(&buf, "RELATION: %s (columns: %ld, count: %ld)", relation->name, relation->data->size(), relation->data->at(0)->data->size());
            appendStringInfo(&buf, "sample_values: %s", get_sample_values(relation->data));
        }
        else if (node->tag == T_TP_JOIN)
        {
            const TraceProvJoinExpr *join_expr = (TraceProvJoinExpr *)node;
            appendStringInfo(&buf, "%s", traceprov_node_to_string(join_expr->left));
            appendStringInfoString(&buf, " JOIN ");
            appendStringInfo(&buf, "%s", traceprov_node_to_string(join_expr->right));
            appendStringInfoString(&buf, " - OUT: [");
            if (join_expr->output_columns != nullptr)
            {
                for (long unsigned int i = 0; i < join_expr->output_columns->size(); i++)
                {
                    auto out_column = join_expr->output_columns->at(i);
                    appendStringInfo(&buf, "(%ld, %ld)", out_column->first, out_column->second);
                }
            }

            appendStringInfoString(&buf, "]");
            appendStringInfoString(&buf, " - ON: [");
            for (long unsigned int i = 0; i < join_expr->join_condition->size(); i++)
            {
                auto out_column = join_expr->join_condition->at(i);
                appendStringInfo(
                    &buf,
                    "(%ld, %ld)=(%ld, %ld)",
                    out_column->first->first,
                    out_column->first->second,
                    out_column->second->first,
                    out_column->second->second);
            }
            appendStringInfoString(&buf, "]");
            appendStringInfo(&buf, "(is_left_star: %d, is_right_star: %d)", join_expr->is_left_star, join_expr->is_right_star);

            if (join_expr->result != nullptr)
            {
                appendStringInfo(&buf, " - RESULT_SIZE: [%ld]", join_expr->result->at(0)->data->size());
                appendStringInfo(&buf, "sample_values: %s", get_sample_values(join_expr->result));
            }
        }
        else if (node->tag == T_TP_APPEND)
        {
            const TraceProvAppend *tp_append = (TraceProvAppend *)node;
            appendStringInfo(&buf, "APPEND ( is_lazy: %d) [", tp_append->is_lazy);
            ListCell *node_cursor;
            foreach (node_cursor, tp_append->nodes)
            {
                TraceProvNode *child_node = (TraceProvNode *)lfirst(node_cursor);
                appendStringInfoString(&buf, traceprov_node_to_string(child_node));
            }
            appendStringInfoString(&buf, "]");
        }
        else
        {
            elog(ERROR, "Unrecognized node!");
        }
        appendStringInfoChar(&buf, ')');
        return buf.data;
    }

    typedef TraceProvData TraceProvJoinResult;
    typedef std::pair<uint64, uint64> TraceProvTuple;
    // Evaluates the join exprn
    static TraceProvJoinResult *traceprov_evaluate_join_exprn_key_count_1(
        TraceProvColumnData *left_column,
        TraceProvColumnData *right_column,
        bool is_single_result)
    {
        // Basically, return the offsets that are found.
        auto offsets_found = new TraceProvJoinResult;
        auto left_offsets = traceprov_make_empty_column();
        auto right_offsets = traceprov_make_empty_column();

        offsets_found->push_back(left_offsets);
        offsets_found->push_back(right_offsets);
        // Trivial case.
        if (left_column->data->size() == 0 || right_column->data->size() == 0)
            return offsets_found;

        // eh.
        left_offsets->data->reserve(left_column->data->size() / 2);
        right_offsets->data->reserve(right_column->data->size() / 2);

        std::vector<TraceProvTuple> *left_column_clone = new std::vector<TraceProvTuple>;
        left_column_clone->reserve(left_column->data->size());
        for (uint64 left_offset = 0; left_offset < left_column->data->size(); left_offset++)
        {
            left_column_clone->push_back(TraceProvTuple(left_column->data->at(left_offset), left_offset));
        }

        std::sort(left_column_clone->begin(), left_column_clone->end());
        // const auto evaluate_start = std::chrono::steady_clock::now();
        for (uint64 offset = 0; offset < right_column->data->size(); offset++)
        {
            const auto match_key = right_column->data->at(offset);
            auto it = (std::lower_bound(
                left_column_clone->begin(),
                left_column_clone->end(),
                match_key,
                [](const TraceProvTuple &p, const uint64 val)
                {
                    return p.first < val;
                }));

            while (it != left_column_clone->end())
            {
                bool has_seen_before = false;
                if (it->first == match_key)
                {
                    if (has_seen_before && is_single_result)
                        elog(ERROR, "Got multiple matches!");
                    auto left_offset = it->second;
                    left_offsets->data->push_back(left_offset);
                    right_offsets->data->push_back(offset);
                    it++;
                    has_seen_before = true;
                    continue;
                }
                break;
            }
        }
        return offsets_found;
    }

    uint64 traceprov_get_node_column_count(TraceProvNode *node)
    {
        if (node->tag == T_TP_RELATION)
        {
            TraceProvRelation *relation = (TraceProvRelation *)node;
            return relation->data->size();
        }
        else if (node->tag == T_TP_JOIN)
        {
            TraceProvJoinExpr *join_exprn = (TraceProvJoinExpr *)node;
            uint64 left_count = 0, right_count = 0, mid_count = 0;
            if (join_exprn->is_left_star)
            {
                left_count = traceprov_get_node_column_count(join_exprn->left);
            }
            if (join_exprn->is_right_star)
            {
                right_count = traceprov_get_node_column_count(join_exprn->right);
            }
            if (join_exprn->output_columns != nullptr)
            {
                for (auto output_column : *join_exprn->output_columns)
                {
                    if (
                        (output_column->first == 1 && join_exprn->is_left_star) ||
                        (output_column->first == 2 && join_exprn->is_right_star))
                    {
                        elog(INFO, "Skipping because of left/right *");
                        continue;
                    }
                    if (output_column->first == 1)
                        left_count = 0;
                    if (output_column->first == 2)
                        right_count = 0;
                    mid_count++;
                }
            }

            return left_count + mid_count + right_count;
        }
        else if (node->tag == T_TP_APPEND)
        {
            TraceProvAppend *append = (TraceProvAppend *)node;
            return traceprov_get_node_column_count((TraceProvNode *)list_nth(append->nodes, 0));
        }
        else if (node->tag == T_TP_WINDOW_READ)
        {
            TraceProvWindowRead *window_read = (TraceProvWindowRead *)node;
            return window_read->column_count;
        }
        else if (node->tag == T_TP_FILTER)
        {
            TraceProvFilter *filter = (TraceProvFilter *)node;
            return traceprov_get_node_column_count(filter->child_node);
        }
        else if (node->tag == T_TP_EXISTS)
        {
            TraceProvExists *exists = (TraceProvExists *)node;
            return traceprov_get_node_column_count(exists->current);
        }
        elog(ERROR, "Got unexepected node: %d", node->tag);
        return 0;
    }

    static char *traceprov_get_column_name_idx(char *alias, uint64 column_idx)
    {
        if (alias == nullptr)
            return psprintf("column_%ld", column_idx);
        return psprintf("%s.column_%ld", alias, column_idx);
    }

    static char *traceprov_get_column_select(char *alias, uint64 column_count, bool add_bigint_cast = false)
    {
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        for (uint64 column_idx = 0; column_idx < column_count; column_idx++)
        {
            if (column_idx > 0)
                appendStringInfoChar(&sql_repr, ',');
            appendStringInfoString(&sql_repr, traceprov_get_column_name_idx(alias, column_idx));
            if (add_bigint_cast)
            {
                appendStringInfoString(&sql_repr, "::bigint");
            }
        }
        return sql_repr.data;
    }

    static List *traceprov_get_indexes(TraceProvNode *node)
    {
        if (node->tag == T_TP_RELATION)
        {
            if (node->alias_name == NULL)
                elog(ERROR, "Expected alias to be set, so far!");
            TraceProvRelation *relation = (TraceProvRelation *)node;
            char *create_index = psprintf("create index if not exists %s_idx on %s (%s)", node->alias_name, relation->name, traceprov_get_column_select(nullptr, relation->data->size()));
            PRINT_ON_VALIDATE("IDX: %s", create_index);
            List *create_indexes = list_make1(
                create_index);
            return create_indexes;
            // return NIL;
        }
        if (node->tag == T_TP_APPEND)
        {
            TraceProvAppend *append = (TraceProvAppend *)node;
            List *indexes = NIL;
            ListCell *cursor;
            foreach (cursor, append->nodes)
            {
                indexes = list_concat(indexes, traceprov_get_indexes((TraceProvNode *)lfirst(cursor)));
            }
            return indexes;
        }
        if (node->tag == T_TP_JOIN)
        {
            TraceProvJoinExpr *join_expr = (TraceProvJoinExpr *)node;
            List *indexes = traceprov_get_indexes(join_expr->left);
            indexes = list_concat(indexes, traceprov_get_indexes(join_expr->right));
            // std::string left_idx = "";
            // std::string right_idx = "";
            // for (auto join_pair: *join_expr->join_condition){
            //     if (join_expr->left->tag == T_TP_RELATION){
            //         if (left_idx != "") left_idx += ",";
            //         left_idx += traceprov_get_column_name_idx(nullptr, join_pair->first->second - 1);
            //     }
            //     if (join_expr->right->tag == T_TP_RELATION){
            //         if (right_idx != "") right_idx += ",";
            //         right_idx += traceprov_get_column_name_idx(nullptr, join_pair->second->second - 1);
            //     }
            // }
            // if (left_idx != ""){
            //     TraceProvRelation *left_relation = (TraceProvRelation *)join_expr->left;
            //     char *create_index = psprintf("create index if not exists %s_idx on %s (%s)", left_relation->alias_name, left_relation->name, left_idx.c_str());
            //     indexes = lappend(indexes, create_index);
            // }
            // if (right_idx != ""){
            //     TraceProvRelation *right_relation = (TraceProvRelation *)join_expr->right;
            //     char *create_index = psprintf("create index if not exists %s_idx on %s (%s)", right_relation->alias_name, right_relation->name, right_idx.c_str());
            //     indexes = lappend(indexes, create_index);
            // }
            return indexes;
        }
        elog(ERROR, "invalid node: %d", node->tag);
        return NIL;
    }

    static char *traceprov_relation_to_sql(TraceProvRelation *relation, TraceProvToSQLContext context)
    {
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfoString(&sql_repr, "SELECT ");
        appendStringInfoString(&sql_repr, traceprov_get_column_select(relation->name, relation->data->size(), false));
        if (context.use_table_def)
        {
            if (relation->rel_args == NULL)
                elog(INFO, "For table defs, expected the rel args to be filled!");
            appendStringInfo(&sql_repr, " FROM traceprov_read_worker_layer(%d::int, %d::int) AS %s", relation->rel_args->worker_id, relation->rel_args->layer_number, relation->name);
        }
        else
        {
            appendStringInfo(&sql_repr, " FROM %s", relation->name);
        }
        return sql_repr.data;
    }

    static char *expand_alias(const char *alias_name, const uint64 column_count)
    {
        // Expands alias such that it is as table(col0, col1, col2...)
        StringInfoData alias_repr;
        initStringInfo(&alias_repr);
        auto select = traceprov_get_column_select(nullptr, column_count);
        appendStringInfo(&alias_repr, "%s(%s)", alias_name, select);
        return alias_repr.data;
    }

    static char *traceprov_window_read_to_sql(TraceProvWindowRead *window_read, TraceProvToSQLContext context)
    {
        char *child_raw_sql = traceprov_node_to_sql(window_read->child_node, context);
        char *child_alias = window_read->child_node->alias_name;
        const uint64 child_col_count = traceprov_get_node_column_count(window_read->child_node);
        char *child_alias_expanded = expand_alias(child_alias, child_col_count);
        char *child_node_sql = psprintf("(%s) as %s", child_raw_sql, child_alias_expanded);
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfoString(&sql_repr, "SELECT *");
        appendStringInfoChar(&sql_repr, ',');
        TraceProvRelationArgs *rel_arg = window_read->rel_args;
        int64 log_col_count = window_read->column_count - child_col_count;
        if (log_col_count < 0)
            elog(ERROR, "Expected log count to always have >= 0 col count!");
        appendStringInfo(
            &sql_repr,
            "unnest(traceprov_read_window_%ld(%d, %d, %s, %s), recursive := true)",
            log_col_count,
            rel_arg->worker_id,
            rel_arg->layer_number,
            traceprov_get_column_name_idx(child_alias, window_read->frame_start_idx),
            traceprov_get_column_name_idx(child_alias, window_read->frame_end_idx));
        appendStringInfo(&sql_repr, " FROM %s ", child_node_sql);
        return sql_repr.data;
    }

    static char *traceprov_join_to_sql(TraceProvJoinExpr *join_expr, TraceProvToSQLContext context)
    {
        char *left_node_raw_sql = traceprov_node_to_sql(join_expr->left, context);
        char *right_node_raw_sql = traceprov_node_to_sql(join_expr->right, context);
        char *left_alias = join_expr->left->alias_name;
        char *right_alias = join_expr->right->alias_name;
        char *left_alias_expanded = expand_alias(left_alias, traceprov_get_node_column_count(join_expr->left));
        char *right_alias_expanded = expand_alias(right_alias, traceprov_get_node_column_count(join_expr->right));

        char *left_node_sql = psprintf("(%s) as %s", left_node_raw_sql, left_alias_expanded);
        char *right_node_sql = psprintf("(%s) as %s", right_node_raw_sql, right_alias_expanded);
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfoString(&sql_repr, "SELECT ");
        bool did_append = false;

        // Expand all lefts.
        if (join_expr->is_left_star)
        {
            did_append = true;
            const uint64 left_column_count = traceprov_get_node_column_count(join_expr->left);
            appendStringInfoString(&sql_repr, traceprov_get_column_select(left_alias, left_column_count));
        }

        // Expand all the rights.
        if (join_expr->is_right_star)
        {
            if (did_append)
                appendStringInfoString(&sql_repr, ", ");
            did_append = true;
            const uint64 right_column_count = traceprov_get_node_column_count(join_expr->right);
            appendStringInfoString(&sql_repr, traceprov_get_column_select(right_alias, right_column_count));
        }

        if (join_expr->output_columns != nullptr)
        {
            for (auto output_column : *join_expr->output_columns)
            {
                if (
                    (output_column->first == 1 && join_expr->is_left_star) ||
                    (output_column->first == 2 && join_expr->is_right_star))
                {
                    elog(INFO, "Skipping because of left/right *");
                    continue;
                }
                if (did_append)
                    appendStringInfoString(&sql_repr, ", ");
                auto alias_name = output_column->first == 1 ? left_alias : right_alias;
                appendStringInfoString(&sql_repr, traceprov_get_column_name_idx(alias_name, output_column->second - 1));
                did_append = true;
            }
        }
        // Add the from claause.
        StringInfoData join_repr;
        initStringInfo(&join_repr);
        bool did_add_in_join = false;
        for (auto join_condition : *join_expr->join_condition)
        {
            if (did_add_in_join)
                appendStringInfoString(&join_repr, " AND ");
            did_add_in_join = true;
            auto left_column = join_condition->first;
            auto right_column = join_condition->second;
            if (VALIDATE_MODE)
            {
                if (left_column->first != 1 || right_column->first != 2)
                    elog(ERROR, "Got invalid numbering!");
            }
            appendStringInfo(
                &join_repr,
                "%s=%s",
                traceprov_get_column_name_idx(left_alias, left_column->second - 1),
                traceprov_get_column_name_idx(right_alias, right_column->second - 1));
        }
        for (auto join_condition : *join_expr->const_join_condition)
        {
            if (did_add_in_join)
                appendStringInfoString(&join_repr, " AND ");
            did_add_in_join = true;
            auto left_column = join_condition->first;
            auto right_column = join_condition->second;
            if (VALIDATE_MODE)
            {
                if (left_column->first != 1)
                    elog(ERROR, "Got invalid numbering!");
            }
            appendStringInfo(
                &join_repr,
                "%s=%ld",
                traceprov_get_column_name_idx(left_alias, left_column->second - 1),
                right_column);
        }
        appendStringInfo(&sql_repr, " FROM %s JOIN %s ON (%s) ", left_node_sql, right_node_sql, join_repr.data);
        return sql_repr.data;
    }

    static char *traceprov_append_to_sql(TraceProvAppend *append, TraceProvToSQLContext context)
    {
        if (list_length(append->nodes) == 1)
        {
            TraceProvNode *single_node = (TraceProvNode *)list_nth(append->nodes, 0);
            char *node_sql = traceprov_node_to_sql(single_node, context);
            // Don't bother generating a new alias for this specific case.
            // Just use whatever the child is.
            append->alias_name = single_node->alias_name;
            return node_sql;
        }
        append->alias_name = tp_parse_get_unique_alias(context.context);
        StringInfoData append_repr;
        initStringInfo(&append_repr);
        ListCell *node_cursor;
        foreach (node_cursor, append->nodes)
        {
            if (foreach_current_index(node_cursor) > 0)
                appendStringInfoString(&append_repr, " UNION ALL ");
            TraceProvNode *node = (TraceProvNode *)lfirst(node_cursor);
            char *node_sql = traceprov_node_to_sql(node, context);
            appendStringInfo(&append_repr, " (%s) ", node_sql);
        }
        return append_repr.data;
    }

    static char *traceprov_filter_to_sql(TraceProvFilter *filter, TraceProvToSQLContext context)
    {
        char *child_raw_sql = traceprov_node_to_sql(filter->child_node, context);
        char *child_alias = filter->child_node->alias_name;
        const uint64 child_col_count = traceprov_get_node_column_count(filter->child_node);
        char *child_alias_expanded = expand_alias(child_alias, child_col_count);
        char *child_node_sql = psprintf("(%s) as %s", child_raw_sql, child_alias_expanded);

        StringInfoData where_repr;
        initStringInfo(&where_repr);
        bool did_add_in_join = false;
        for (auto join_pair : *filter->const_join_condition)
        {
            if (did_add_in_join)
                appendStringInfoString(&where_repr, " AND ");
            did_add_in_join = true;
            appendStringInfo(
                &where_repr,
                "(%s=%ld)",
                traceprov_get_column_name_idx(child_alias, join_pair->first->second - 1),
                join_pair->second);
        }

        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfo(&sql_repr, "SELECT * FROM %s WHERE (%s)", child_node_sql, where_repr.data);
        return sql_repr.data;
    }

    static char *traceprov_exists_to_sql(TraceProvExists *exists, TraceProvToSQLContext context)
    {
        char *condition_sql = traceprov_node_to_sql(exists->condition, context);
        char *current_sql = traceprov_node_to_sql(exists->current, context);
        const uint64 current_col_count = traceprov_get_node_column_count(exists->current);
        char *current_alias_expanded = expand_alias(exists->current->alias_name, current_col_count);

        char *sql = psprintf("select * from (%s) as %s where (exists (%s))", current_sql, current_alias_expanded, condition_sql);
        return sql;
    }

    static char *traceprov_node_to_sql(TraceProvNode *node, TraceProvToSQLContext context)
    {
        if (node->tag == T_TP_RELATION)
        {
            node->alias_name = tp_parse_get_unique_alias(context.context);
            return traceprov_relation_to_sql((TraceProvRelation *)node, context);
        }
        if (node->tag == T_TP_JOIN)
        {
            node->alias_name = tp_parse_get_unique_alias(context.context);
            return traceprov_join_to_sql((TraceProvJoinExpr *)node, context);
        }
        if (node->tag == T_TP_APPEND)
        {
            return traceprov_append_to_sql((TraceProvAppend *)node, context);
        }
        if (node->tag == T_TP_WINDOW_READ)
        {
            node->alias_name = tp_parse_get_unique_alias(context.context);
            return traceprov_window_read_to_sql((TraceProvWindowRead *)node, context);
        }
        if (node->tag == T_TP_FILTER)
        {
            node->alias_name = tp_parse_get_unique_alias(context.context);
            return traceprov_filter_to_sql((TraceProvFilter *)node, context);
        }
        if (node->tag == T_TP_EXISTS)
        {
            node->alias_name = tp_parse_get_unique_alias(context.context);
            return traceprov_exists_to_sql((TraceProvExists *)node, context);
        }
        elog(ERROR, "Found handling invalid node: %d", node->tag);
    }

    static void get_used_sublinks_in_dependency(
        const TraceProvDependency *dependency,
        TraceProvParseContext *tp_context,
        TraceProvEntryCountMap *entry_count_map,
        List **sublink_ref,
        TraceProvLastRef *last_ref)
    {
        ListCell *cursor;
        foreach (cursor, dependency->entries)
        {
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            // TODO: Handle set operations here too.
            if (entry->kind == TP_ENTRY_KIND_POINTER)
            {
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(dependency->children, foreach_current_index(cursor));
                get_used_sublinks_in_dependency(child_graph, tp_context, entry_count_map, sublink_ref, last_ref);
            }
            ListCell *sublink_cursor;
            foreach (sublink_cursor, entry->sublinks)
            {
                TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_cursor);
                const TraceProvLayerNumber sublink_number = item->layer_number;
                // Append to the sublinks found.
                *sublink_ref = list_append_unique_int(*sublink_ref, sublink_number);
                if (entry_count_map->find(sublink_number) == entry_count_map->end())
                {
                    entry_count_map->insert({sublink_number, 0});
                }
                entry_count_map->at(sublink_number)++;
                if (item->offset_in_key == 0)
                {
                    // This means that the current sublink is the last ref to the target sublink.
                    // Cache that info.
                    if (last_ref->find(sublink_number) != last_ref->end())
                    {
                        elog(ERROR, "Didn't expect to find :%d again!", sublink_number);
                    }
                    last_ref->insert({sublink_number, dependency->headNumber});
                }
            }
        }
    }

    static void get_used_sublinks(
        List *graphs,
        TraceProvParseContext *tp_context,
        TraceProvLayerEntryMap *p_layer_count_map,
        TraceProvDerivableSublinkMap *p_derivable_map,
        TraceProvLastRef *last_ref)
    {
        // Sublinks can refer to other sublinks.
        // So, discover those cases too.
        ListCell *graph_cursor;
        foreach (graph_cursor, graphs)
        {
            const auto graph = (TraceProvDependency *)lfirst(graph_cursor);
            List *used_sublinks = NIL;
            auto graph_entry_count_map = new TraceProvEntryCountMap;
            get_used_sublinks_in_dependency(
                graph,
                tp_context,
                graph_entry_count_map,
                &used_sublinks,
                last_ref);
            p_layer_count_map->insert({graph->headNumber, graph_entry_count_map});
            p_derivable_map->insert({graph->headNumber, used_sublinks});
        }
    }

    static List *get_directly_derivable(const TraceProvDependency *graph, const TraceProvParseContext *parse_context)
    {
        // All the entries that are directly derivable.
        List *derivable = NIL;
        derivable = lappend_int(derivable, graph->headNumber);
        ListCell *cursor;
        foreach (cursor, graph->entries)
        {
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            // TODO: Handle set operations here too.
            if (entry->kind == TP_ENTRY_KIND_POINTER)
            {
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(graph->children, foreach_current_index(cursor));
                derivable = list_concat(derivable, get_directly_derivable(child_graph, parse_context));
            }
            ListCell *sublink_cursor;
            foreach (sublink_cursor, entry->sublinks)
            {
                TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_cursor);
                TraceProvDependency *sublink_graph = tp_get_sublink_graph(parse_context, item->layer_number);
                derivable = list_concat(derivable, get_directly_derivable(sublink_graph, parse_context));
            }
        }
        return derivable;
    }

    // Checks if two derivable sublink maps are the same or not.
    static bool derivable_equivalent(const TraceProvDerivableSublinkMap *base, const TraceProvDerivableSublinkMap *other)
    {
        if (base->size() != other->size())
        {
            // This doesn't make much sense in handling this case, but does help making code cleaner later.
            return false;
        }
        for (auto base_pair : *base)
        {
            const List *base_derivable = base_pair.second;
            if (other->find(base_pair.first) == other->end())
            {
                elog(ERROR, "Expected to find %d in the other!", base_pair.first);
            }
            const List *other_derivable = other->at(base_pair.first);
            const int base_length = list_length(base_derivable);
            const int other_length = list_length(other_derivable);
            if (base_length != other_length)
            {
                return false;
            }
            const List *intersect = list_intersection_int(base_derivable, other_derivable);
            if (list_length(intersect) != base_length)
                return false;
        }
        return true;
    }

    static TraceProvDerivableSublinkMap *extend_reachable(TraceProvDerivableSublinkMap *source)
    {
        auto extended = new TraceProvDerivableSublinkMap;
        auto last_result = source;
        while (true)
        {
            if (extended->size() == 0)
            {
                for (auto e_pair : *last_result)
                {
                    extended->insert({e_pair.first, list_copy(e_pair.second)});
                }
            }
            for (auto res_pair : *extended)
            {
                extended->at(res_pair.first) = list_union_int(res_pair.second, last_result->at(res_pair.first));
            }
            if (derivable_equivalent(last_result, extended))
            {
                return extended;
            }
            last_result = extended;
        }
        return NULL;
    }

    typedef std::unordered_map<TraceProvLayerNumber, List *> TraceProvLayerEntriesMap;
    // Essentially, create a hash map of all the entries.
    static void create_layer_entry_map(const TraceProvDependency *graph, TraceProvLayerEntriesMap *entries_map)
    {
        entries_map->insert({graph->headNumber, graph->entries});
        ListCell *cursor;
        foreach (cursor, graph->entries)
        {
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            // TODO: Handle set operations here too.
            if (entry->kind == TP_ENTRY_KIND_POINTER)
            {
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(graph->children, foreach_current_index(cursor));
                create_layer_entry_map(child_graph, entries_map);
            }
        }
    }

    uint32 count_correlated_attrs(const List *entries)
    {
        uint32 correlated_attr = 0;
        ListCell *entry_cursor;
        foreach (entry_cursor, entries)
        {
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->kind == TP_ENTRY_CORRELATION_ATTR || te->kind == TP_ENTRY_IN_CORRELATION_ATTR)
                continue;
            correlated_attr++;
        }
        return correlated_attr;
    }

    // static derive_from_log(const uint8 worker_count)
    // The main entry point to all the derivation.
    PG_FUNCTION_INFO_V1(traceprov_perform_derivation);

    Datum traceprov_perform_derivation(PG_FUNCTION_ARGS)
    {
        return (Datum)0;
    }

    static void traceprov_materialize_raw_data(Tuplestorestate *tupstore, TupleDesc tupdesc, TraceProvData *data)
    {
        const uint32 num_attrs = data->size();
        Datum *record = (Datum *)palloc0(sizeof(Datum) * num_attrs);
        bool *nulls = palloc0_array(bool, num_attrs);

        for (uint64 row_idx = 0; row_idx < data->at(0)->data->size(); row_idx++)
        {
            memset(nulls, 0, sizeof(bool) * num_attrs);
            for (uint32 col_idx = 0; col_idx < num_attrs; col_idx++)
            {
                record[col_idx] = Int64GetDatum(data->at(col_idx)->data->at(row_idx));
                nulls[col_idx] = !data->at(col_idx)->validity->at(row_idx);
            }
            tuplestore_putvalues(tupstore, tupdesc, record, nulls);
        }
    }

    // Generic prepare for materialization.
    // Used for copy-based, and deep-copy-based methods.
    // Since we end up storing the tupstore and tupdesc in rsinfo, we don't need any return (or out pointers.)
    static void traceprov_prepare_for_materialize(
        FunctionCallInfo fcinfo,
        const int32 width)
    {
        ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
        TupleDesc tupdesc;
        Tuplestorestate *tupstore;
        MemoryContext per_query_ctx;
        MemoryContext oldcontext;

        /* check to see if caller supports us returning a tuplestore */
        if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("set-valued function called in context that cannot accept a set")));
        if (!(rsinfo->allowedModes & SFRM_Materialize))
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("materialize mode required, but it is not allowed in this context")));

        per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
        oldcontext = MemoryContextSwitchTo(per_query_ctx);
        const int32 num_attrs = width;
        tupdesc = CreateTemplateTupleDesc(num_attrs);
        for (int32 col_idx = 0; col_idx < num_attrs; col_idx++)
        {
            TupleDescInitEntry(tupdesc, (AttrNumber)col_idx + 1, psprintf("column_%d", col_idx), INT8OID, -1, 0);
        }

        tupstore = tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random, false, 10);
        rsinfo->returnMode = SFRM_Materialize;
        rsinfo->setResult = tupstore;
        rsinfo->setDesc = tupdesc;
        MemoryContextSwitchTo(oldcontext);
    }

    // Based off pg_prepared_statement.
    static void traceprov_materialize_derived_result(FunctionCallInfo fcinfo, TraceProvTopResult *derived)
    {
        traceprov_prepare_for_materialize(fcinfo, derived->width);
        ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
        TupleDesc tupdesc = rsinfo->setDesc;
        Tuplestorestate *tupstore = rsinfo->setResult;
        ListCell *data_cursor;
        foreach (data_cursor, derived->pdata)
        {
            TraceProvData *data = (TraceProvData *)lfirst(data_cursor);
            traceprov_materialize_raw_data(tupstore, tupdesc, data);
        }
#if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
#endif
        return;
    }

    PG_FUNCTION_INFO_V1(traceprov_dump_derivation);

    Datum traceprov_dump_derivation(PG_FUNCTION_ARGS)
    {
        PG_RETURN_INT64(0);
    }

    PG_FUNCTION_INFO_V1(traceprov_derivation_spec);

    Datum traceprov_derivation_spec(PG_FUNCTION_ARGS)
    {
        PG_RETURN_TEXT_P(cstring_to_text("{}"));
    }

    PG_FUNCTION_INFO_V1(traceprov_get_sql_derivation);

    Datum traceprov_get_sql_derivation(PG_FUNCTION_ARGS)
    {
        PG_RETURN_TEXT_P(cstring_to_text("{}"));
    }

    // Calls read_all_columns multiple times
    // and reuturs json of time taken.
    PG_FUNCTION_INFO_V1(traceprov_perf_read);
    Datum traceprov_perf_read(PG_FUNCTION_ARGS)
    {
        const TraceProvLayerNumber input_layer_number = PG_GETARG_INT32(0);
        const uint64 repeat = PG_GETARG_INT64(1);
        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping shared context");
        }

        const auto worker_local_contexts = traceprov_get_local_contexts(context.worker_count);
        // We really don't need more than 1.
        if (worker_local_contexts->size() != 1)
            elog(ERROR, "Got invalid worker local contexts..");

        const auto worker_context = worker_local_contexts->at(0);
        const auto layer = &worker_context->cached_layers[input_layer_number - 1];
        if (layer->layer_number != input_layer_number)
            elog(ERROR, "Expected layer number to be set!");

        typedef struct ReadResult
        {
            uint64 read_size;
            uint64 time;
            uint64 width;
        } ReadResult;

        auto read_record_spec = new std::vector<ReadResult>;
        read_record_spec->reserve(repeat);

        for (uint64 read_idx = 0; read_idx < repeat; read_idx++)
        {
            TP_EVALUATE_START();
            auto all_records = read_all_columns(input_layer_number, worker_local_contexts, nullptr);
            TP_EVALUATE_END();
            read_record_spec->emplace_back(ReadResult{
                .read_size = all_records->at(0)->data->size(),
                .time = (uint64)TP_EVALUATE_DURATION(),
                .width = all_records->size()});
            // NOTE: Temporarily disabled, to check against duckdb..
            // for (auto column: *all_records){
            //     // Delete all the column data (for efficieny reasons.)
            //     delete column->data;
            // }
        }

        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoString(&buf, "[");
        for (uint64 read_idx = 0; read_idx < repeat; read_idx++)
        {
            if (read_idx > 0)
                appendStringInfoString(&buf, ",");
            appendStringInfo(
                &buf,
                "{\"read_size\": %ld, \"time\": %ld, \"width\": %ld}",
                read_record_spec->at(read_idx).read_size,
                read_record_spec->at(read_idx).time,
                read_record_spec->at(read_idx).width);
        }
        appendStringInfoString(&buf, "]");
        delete read_record_spec;
        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }

    static void flattenTraceProvInferAbstractTree(TraceProvInferAbstractTree *tree, TraceProvResultMap *result_map, TraceProvParseContext *parse_context)
    {
        List *copied = NIL;
        for (auto val : *tree->nodes)
        {
            copied = lappend(copied, val);
        }

        if (list_length(copied) > 0)
        {
            TraceProvNode *new_append = make_traceprov_append(copied, false);
            if ((result_map->find(tree->layer_number) == result_map->end()))
            {
                result_map->insert({tree->layer_number, new_append});
            }
            else
            {
                TraceProvNode *old_node = result_map->at(tree->layer_number);
                if (old_node->tag == T_TP_APPEND)
                {
                    TraceProvAppend *old_append = (TraceProvAppend *)old_node;
                    old_append->nodes = list_concat(old_append->nodes, copied);
                }
                else
                {
                    result_map->at(tree->layer_number) = make_traceprov_append(
                        list_make2(old_node, new_append),
                        false);
                }
            }
        }

        for (auto child : *tree->children)
        {
            flattenTraceProvInferAbstractTree(child, result_map, parse_context);
        }
    }

    std::vector<TraceProvWorkerLayer> *find_layers_across_workers(
        TraceProvLayerNumber log_layer_number,
        const std::vector<struct local_context *> *worker_local_contexts,
        const uint32 expected_layer_width,
        const bool do_strict)
    {
        auto found_worker_layers = new std::vector<TraceProvWorkerLayer>;
        // Can have, at most, the number of workers.
        found_worker_layers->reserve(worker_local_contexts->size());
        for (auto worker_local_context : *worker_local_contexts)
        {
            struct traceprov_aggregate_layer *candidate_layer = &worker_local_context->cached_layers[log_layer_number - 1];
            if (candidate_layer->layer_number == 0)
                continue;
            // If it is not 0, it should always be the current log layer...
            if (candidate_layer->layer_number != log_layer_number)
                elog(ERROR, "Got invalid log state!");

            // Don't include layers that have 0 as the size (all the layers must have at least 1 page, if inited
            // via local_init. But, those synthetically created won't, and should be excluded anyways.
            if (candidate_layer->size == 0 && do_strict)
                continue;
            if (expected_layer_width > 0 && (candidate_layer->num_pk_records != expected_layer_width))
                elog(ERROR, "Expeced the width to be consistent!");

            found_worker_layers->emplace_back(TraceProvWorkerLayer(worker_local_context->worker_id, candidate_layer));
        }

        if (found_worker_layers->size() == 0)
            elog(ERROR, "Expected the log to always be found!");

        PRINT_ON_VALIDATE("Found %ld log_layers for %d layer number", found_worker_layers->size(), log_layer_number);
        return found_worker_layers;
    }

    uint32 get_reference_count(const TraceProvLayerEntryMap *layer_entry_map, const TraceProvLayerNumber query_layer_num)
    {
        uint32 reference_count = 0;
        // TODO: Optimize this call away??
        for (auto layer_map_pair : *layer_entry_map)
        {
            auto candidate = layer_map_pair.second;
            if (candidate->find(query_layer_num) == candidate->end())
                continue;
            reference_count += candidate->at(query_layer_num);
        }
        return reference_count;
    }

    List *get_referring_layers(const TraceProvDerivableSublinkMap *derivable_sublink_map, const TraceProvLayerNumber sublink)
    {
        List *referring_layers = NIL;
        for (auto derivable_pair : *derivable_sublink_map)
        {
            if (list_member_int(derivable_pair.second, sublink))
            {
                referring_layers = lappend_int(referring_layers, derivable_pair.first);
            }
        }
        return referring_layers;
    }

    static TraceProvRecursePack shallow_copy_recurse_pack(TraceProvRecursePack *reference, const bool copy_sublinks = false)
    {
        TraceProvPendingSublinks *new_sublinks = new TraceProvPendingSublinks;
        // that's why you don't try constructing TraceProvRecursePack on the fly kids!
        for (auto pair : *reference->pending_sublinks)
        {
            new_sublinks->insert({pair.first, list_copy(pair.second)});
        }
        TraceProvEntryCountMap *entry_count_map = new TraceProvEntryCountMap;
        for (auto pair : *reference->entry_count_map)
        {
            entry_count_map->insert({pair.first, pair.second});
        }
        return TraceProvRecursePack{
            .pending_sublinks = new_sublinks,
            .size_layer_map = reference->size_layer_map,
            .layer_count_map = reference->layer_count_map,
            .derivable_map = reference->derivable_map,
            .base_derivable_map = reference->base_derivable_map,
            .current_layer_number = reference->current_layer_number,
            .derived_sublinks = reference->derived_sublinks,
            .entry_count_map = entry_count_map,
            .last_ref = reference->last_ref,
            .sublinks_to_ignore = reference->sublinks_to_ignore,
            .combine_main_worker = reference->combine_main_worker};
    }

    static TraceProvRecursePack increment_recursion(TraceProvRecursePack *reference, const bool copy_sublinks = false)
    {
        // Previously, this used to actually perform an increment on "level"
        // That mess has been reimplemented away :)
        return shallow_copy_recurse_pack(reference, copy_sublinks);
    }

    static TraceProvRecursePack reset_entry_count_map(TraceProvRecursePack *reference, const TraceProvLayerNumber next_ref = 0)
    {
        return TraceProvRecursePack{
            .pending_sublinks = reference->pending_sublinks,
            .size_layer_map = reference->size_layer_map,
            .layer_count_map = reference->layer_count_map,
            .derivable_map = reference->derivable_map,
            .base_derivable_map = reference->base_derivable_map,
            .current_layer_number = next_ref == 0 ? reference->current_layer_number : next_ref,
            .derived_sublinks = new std::vector<TraceProvLayerNumber>,
            .entry_count_map = new TraceProvEntryCountMap,
            .last_ref = reference->last_ref,
            .sublinks_to_ignore = reference->sublinks_to_ignore,
            .combine_main_worker = reference->combine_main_worker};
    }

    static bool check_entry_count_equivalence(const TraceProvEntryCountMap *base, const TraceProvEntryCountMap *target, const bool allow_partial = false)
    {
        if (base->size() != target->size() && (!allow_partial))
            return false;
        for (auto base_pair : *base)
        {
            if (target->find(base_pair.first) == target->end())
                continue;
            if (target->at(base_pair.first) != base_pair.second)
                return false;
        }
        for (auto target_pair : *base)
        {
            if (base->find(target_pair.first) == base->end())
                continue;
            if (base->at(target_pair.first) != target_pair.second)
                return false;
        }
        return true;
    }

    static TraceProvInferAbstractTree *derive_aggregate_on_single_context(
        TraceProvNode *reference_node,
        const uint32 reference_match_idx,
        TraceProvDependency *agg_graph,
        const struct local_context *current_local_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack)
    {
        auto current_tree = makeTraceProvInferAbstractTree(agg_graph->headNumber);
        const TraceProvLayerNumber layer_number_to_search = agg_graph->headNumber;
        const struct traceprov_aggregate_layer *layer = &current_local_context->cached_layers[layer_number_to_search - 1];
        if (layer->layer_number == 0)
            return current_tree;

        auto self_logs = read_all_columns(layer_number_to_search, worker_local_contexts, nullptr, true);
        const uint64 reference_node_col_count = traceprov_get_node_column_count(reference_node);
        const bool aggregate_was_split = layer->is_leader_layer;
        // While we don't need to actually split the data (we can't do that anyways)
        // we still need to read data to make base relations out of it.
        if (!aggregate_was_split)
        {
            // The simple case.
            if (agg_graph->graph_type == TP_PURE_AGGREGATE && (recurse_pack.pending_sublinks->size() == 0) && false)
            {

                auto base_log_relation = make_traceprov_relation(self_logs, psprintf("base_join_%s", tp_parse_get_unique_alias(parse_context)));
                base_log_relation->rel_args = make_relation_args(current_local_context->worker_id, agg_graph->headNumber);
                auto child_tree = derive_on_node(
                    (TraceProvNode *)base_log_relation,
                    agg_graph,
                    0,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    recurse_pack);
                List *derived_nodes = getTraceProvInferAbstractTreeNodes(child_tree);
                ListCell *cursor;
                foreach (cursor, derived_nodes)
                {
                    TraceProvNode **node = (TraceProvNode **)lfirst(cursor);
                    auto exists_node = make_traceprov_exists(*node, reference_node);
                    *node = (TraceProvNode *)exists_node;
                }
                current_tree->children->push_back(child_tree);
            }
            else
            {
                TraceProvJoinExpr *join_exprn = make_traceprov_simple_join(
                    self_logs,
                    (TraceProvNode *)reference_node,
                    parse_context,
                    "intermediate_join",
                    reference_match_idx);

                get_relation_from_join(join_exprn)->rel_args = make_relation_args(recurse_pack.combine_main_worker, layer_number_to_search);

                join_exprn->is_left_star = true;
                current_tree->children->push_back(derive_on_node(
                    (TraceProvNode *)join_exprn,
                    agg_graph,
                    reference_node_col_count,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    recurse_pack));
            }
        }
        else
        {
            // This is a slightly complicated case.
            // But, there is still the guarantee that only 1 worker is the main worker.
            auto layers_across_workers = find_layers_across_workers(agg_graph->headNumber, worker_local_contexts, 0, false);
            TraceProvWorkerLayer leader_layer_pair;
            bool found_leader = false;
            for (auto worker_layer_pair : *layers_across_workers)
            {
                if (worker_layer_pair.second->is_leader_layer)
                {
                    leader_layer_pair = worker_layer_pair;
                    found_leader = true;
                    break;
                }
            }
            if (!found_leader)
                elog(ERROR, "In the split case, always expected to find the leader layer!");

            auto leader_worker_context = worker_local_contexts->at(leader_layer_pair.first - 1);
            TraceProvLayerNumber combine_layer_number = leader_layer_pair.second->combined_aggregate_layer_number;
            std::vector<struct local_context *> leader_worker_context_vec = {leader_worker_context};
            auto combine_logs = read_all_columns(combine_layer_number, &leader_worker_context_vec, nullptr, true);
            TraceProvRelation *combine_relation = make_traceprov_relation(
                combine_logs,
                psprintf("combined_entry"));
            combine_relation->rel_args = make_relation_args(
                recurse_pack.combine_main_worker == 0 ? leader_worker_context->worker_id : recurse_pack.combine_main_worker,
                combine_layer_number);
            // Need to:
            // 1. Join the combine logs to the previous log.
            // 2. Join the combine logs (in an union) to all the local worker logs.
            // 3. Join the base logs directly to the previous log.

            // For combine, need to join on colum 1, but need to propagate column 2 and 3
            // TraceProvColumn *output_column_1 = new TraceProvColumn(2, 2); // This is the worker id
            TraceProvColumn *output_column_2 = new TraceProvColumn(2, 2); // This is the individual log
            auto output_column = new std::vector<TraceProvColumn *>;
            // output_column->push_back(output_column_1);
            output_column->push_back(output_column_2);

            TraceProvColumn *join_column_1 = new TraceProvColumn(1, reference_match_idx);
            TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
            auto join_condition = new TraceProvJoinConditions;
            join_condition->push_back(new std::pair<TraceProvColumn *, TraceProvColumn *>(join_column_1, join_column_2));
            auto combine_join = make_traceprov_join_expr(
                reference_node,
                (TraceProvNode *)combine_relation,
                join_condition,
                output_column,
                true);
            const uint64 combine_match_key_idx = traceprov_get_node_column_count((TraceProvNode *)combine_join);
            if (combine_match_key_idx != (traceprov_get_node_column_count(reference_node) + 1))
                elog(INFO, "Inconsistent state!");
            // const uint64 worker_id_key_idx = combine_match_key_idx - 1;

            for (auto worker_local_pair : *layers_across_workers)
            {
                const uint8 worker_id = worker_local_pair.first;
                auto base_logs = read_all_columns(agg_graph->headNumber, worker_local_contexts, nullptr, true);
                if (base_logs == nullptr)
                    continue;
                auto base_log_relation = make_traceprov_relation(base_logs, psprintf("base_join_%s", tp_parse_get_unique_alias(parse_context)));
                base_log_relation->rel_args = make_relation_args(0, agg_graph->headNumber);
                TraceProvJoinExpr *base_join_exprn = make_traceprov_join_from_rel(
                    reference_node,
                    (TraceProvNode *)base_log_relation,
                    base_log_relation->data->size(),
                    reference_match_idx);
                base_join_exprn->is_left_star = true;
                TraceProvJoinExpr *partial_join_exprn = make_traceprov_join_from_rel(
                    (TraceProvNode *)combine_join,
                    (TraceProvNode *)base_log_relation,
                    base_log_relation->data->size(),
                    combine_match_key_idx);
                // We cannot output everything from the left star (need to only output upto the join columns)
                for (uint col_idx = 0; col_idx < reference_node_col_count; col_idx++)
                {
                    partial_join_exprn->output_columns->insert(
                        partial_join_exprn->output_columns->begin() + col_idx,
                        new TraceProvColumn(1, col_idx + 1));
                }
                // TraceProvColumn *worker_id_column = new TraceProvColumn(1, worker_id_key_idx);
                // partial_join_exprn->const_join_condition->push_back(new TraceProvConstJoinPair(worker_id_column, worker_id));
                if (traceprov_get_node_column_count((TraceProvNode *)partial_join_exprn) != traceprov_get_node_column_count((TraceProvNode *)base_join_exprn))
                {
                    elog(ERROR, "Got mismatching node count on logs!");
                }

                if (agg_graph->graph_type == TP_PURE_AGGREGATE && (recurse_pack.pending_sublinks->size() == 0) && false)
                {
                    auto child_tree = derive_on_node(
                        (TraceProvNode *)base_log_relation,
                        agg_graph,
                        0,
                        current_local_context,
                        worker_local_contexts,
                        parse_context,
                        shallow_copy_recurse_pack(&recurse_pack));
                    List *derived_nodes = getTraceProvInferAbstractTreeNodes(child_tree);
                    ListCell *cursor;
                    foreach (cursor, derived_nodes)
                    {
                        TraceProvNode **node = (TraceProvNode **)lfirst(cursor);
                        // It should be very abornormal to be in a case where we'd get finalized in a child worker.
                        // This is because, even in the case where there's a partition, we're still acting on all the data.
                        auto exists_node = make_traceprov_exists(*node, (TraceProvNode *)partial_join_exprn);
                        *node = (TraceProvNode *)exists_node;
                    }
                    current_tree->children->push_back(child_tree);
                }
                else
                {
                    recurse_pack.combine_main_worker = current_local_context->worker_id;
                    current_tree->children->push_back(derive_on_node(
                        (TraceProvNode *)partial_join_exprn,
                        agg_graph,
                        reference_node_col_count,
                        current_local_context,
                        worker_local_contexts,
                        parse_context,
                        shallow_copy_recurse_pack(&recurse_pack, true)));

                    current_tree->children->push_back(derive_on_node(
                        (TraceProvNode *)base_join_exprn,
                        agg_graph,
                        reference_node_col_count,
                        current_local_context,
                        worker_local_contexts,
                        parse_context,
                        shallow_copy_recurse_pack(&recurse_pack, true)));
                }
                // We're always using UNION-infer optimization here.
                break;
            }
        }

        return current_tree;
    }

    // Performs the derivation
    // from node rather than log. The distinction being that this gets called in nested aggregations.
    static TraceProvInferAbstractTree *derive_aggregate_on_node(
        TraceProvNode *reference_node,
        const uint32 reference_match_idx,
        TraceProvDependency *agg_graph,
        const struct local_context *current_local_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack)
    {
        // const uint64 worker_count = worker_local_contexts->size();
        if ((agg_graph->graph_type != TraceProvGraphKind::TP_AGGREGATE) && (agg_graph->graph_type != TraceProvGraphKind::TP_PURE_AGGREGATE))
            elog(ERROR, "Expected the graph to always be of aggregate!");

        if (current_local_context == NULL)
        {
            // Try figuring out the leader idx for this.
            struct local_context *context = NULL;
            for (auto context : *worker_local_contexts)
            {
                auto layer_number = agg_graph->headNumber;
                const struct traceprov_aggregate_layer *layer = &context->cached_layers[layer_number - 1];
                if (layer->layer_number == 0)
                    continue;
                if (layer->is_leader_layer)
                {
                    current_local_context = context;
                    break;
                }
            }
        }
        if (current_local_context == NULL)
        {
            // If it is still NULL, doesn't matter which one we pick.
            current_local_context = worker_local_contexts->at(0);
        }

        return derive_aggregate_on_single_context(
            reference_node,
            reference_match_idx,
            agg_graph,
            current_local_context,
            worker_local_contexts,
            parse_context,
            recurse_pack);
    }

    static TraceProvInferAbstractTree *derive_window_on_single_context(
        TraceProvNode *reference_node,
        const uint32 frame_start_idx,
        const uint32 frame_end_idx,
        TraceProvDependency *curr_graph,
        const struct local_context *current_local_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack)
    {
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
            reference_node);
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
                increment_recursion(&recurse_pack)));
        if (recurse_pack.size_layer_map->find(log_col_count) == recurse_pack.size_layer_map->end())
        {
            recurse_pack.size_layer_map->insert({log_col_count, new std::vector<TraceProvLayerNumber>});
        }
        auto layer_vector = recurse_pack.size_layer_map->at(log_col_count);
        layer_vector->push_back(layer_number_to_search);
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
        TraceProvRecursePack recurse_pack)
    {
        const int set_start_idx = find_first_set_number_entry(curr_graph->entries, set_number);
        auto current_tree = makeTraceProvInferAbstractTree(curr_graph->headNumber);

        if (set_start_idx == -1)
            return current_tree;

        const List *possible_candidates = tp_get_set_pointer_property(parse_context, set_number);

        ListCell *candidate;
        foreach (candidate, possible_candidates)
        {
            const int curr_set_number = lfirst_int(candidate);
            auto tp_filter = make_traceprov_filter(reference_node);
            tp_filter->const_join_condition->push_back(new TraceProvConstJoinPair(new TraceProvColumn(1, set_idx + idx_start + 1), (uint64)curr_set_number));

            current_tree->children->push_back(
                derive_on_node(
                    (TraceProvNode *)tp_filter,
                    tp_get_set_graph(parse_context, curr_set_number),
                    idx_start + set_start_idx,
                    current_local_context,
                    worker_local_contexts,
                    parse_context,
                    recurse_pack));
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
        TraceProvRecursePack recurse_pack)
    {
        if (current_local_context != NULL)
        {
            return derive_window_on_single_context(
                reference_node,
                frame_start_idx,
                frame_end_idx,
                curr_graph,
                current_local_context,
                worker_local_contexts,
                parse_context,
                recurse_pack);
        }

        auto current_tree = makeTraceProvInferAbstractTree(curr_graph->headNumber);
        for (auto context : *worker_local_contexts)
        {
            current_tree->children->push_back(
                derive_window_on_single_context(
                    reference_node,
                    frame_start_idx,
                    frame_end_idx,
                    curr_graph,
                    context,
                    worker_local_contexts,
                    parse_context,
                    shallow_copy_recurse_pack(&recurse_pack, true)));
        }
        return current_tree;
    }

    void insert_duplicate_safe(std::vector<TraceProvLayerNumber> *in_vec, const TraceProvLayerNumber node_to_insert)
    {
        if (std::find(in_vec->begin(), in_vec->end(), node_to_insert) != in_vec->end())
            return;
        in_vec->push_back(node_to_insert);
    }

    /**
     * Core logic for deriving sublinks.
     */
    static TraceProvInferAbstractTree *derive_sublinks(
        TraceProvNode *node,
        const TraceProvDependency *graph,
        TraceProvParseContext *parse_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        // We cannot assume, indexes are 0-based
        // because we can be in a deep nested-agg chain.
        const uint32 idx_start,
        TraceProvRecursePack recurse_pack)
    {
        const uint32 previous_start_idx = traceprov_get_node_column_count(node);
        auto current_tree = makeTraceProvInferAbstractTree(graph->headNumber);
        ListCell *entry_cursor;
        List *candidate_sublinks = NIL;
        foreach (entry_cursor, graph->entries)
        {
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
            if (list_length(entry->sublinks) == 0)
                continue;
            ListCell *sublink_ref_cursor;
            foreach (sublink_ref_cursor, entry->sublinks)
            {
                const TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_ref_cursor);
                List *pending_entries = NIL;
                if (recurse_pack.pending_sublinks->find(item->layer_number) != recurse_pack.pending_sublinks->end())
                {
                    pending_entries = recurse_pack.pending_sublinks->at(item->layer_number);
                }
                else
                {
                    recurse_pack.pending_sublinks->insert({item->layer_number, NIL});
                }

                const uint32 insert_idx = item->offset_in_key;
                // All indexes are 1-indexed.
                uint32 entry_idx = idx_start + foreach_current_index(entry_cursor) + 1;
                pending_entries = traceprov_set_at_offset_int(pending_entries, insert_idx, entry_idx);
                recurse_pack.pending_sublinks->at(item->layer_number) = pending_entries;
                if (recurse_pack.entry_count_map->find(item->layer_number) == recurse_pack.entry_count_map->end())
                {
                    recurse_pack.entry_count_map->insert({item->layer_number, 0});
                }
                recurse_pack.entry_count_map->at(item->layer_number)++;
                candidate_sublinks = list_append_unique_int(candidate_sublinks, item->layer_number);
            }
        }

        ListCell *candidate_sublink_cursor;
        List *sublinks_to_commit;
        List *reachable_sublinks = NIL;
        const TraceProvEntryCountMap *expected_entry_count_map = recurse_pack.layer_count_map->at(recurse_pack.current_layer_number);
        const TraceProvEntryCountMap *eager_entry_count_map = recurse_pack.entry_count_map;
        // Reversing to handle topological sort :)
        foreach (candidate_sublink_cursor, traceprov_reverse_list(candidate_sublinks))
        {
            TraceProvLayerNumber candidate_sublink = lfirst_int(candidate_sublink_cursor);
            // This will differ from canidate when we utilize a proxy :)
            const TraceProvLayerNumber original_sublink = candidate_sublink;
            // This will happen if the topologically the sublink was derived directly by another upstream.
            auto derived_sublink_vec = *recurse_pack.derived_sublinks;
            if (list_member_int(reachable_sublinks, candidate_sublink))
            {
                // If we did derive it via a topological sort, then it doesn't make sense to add
                // it here. Any sublinks that were derivable later would still be derivable
                // by the "higher" sublink, so this is safe.
                // insert_duplicate_safe(recurse_pack.derived_sublinks, candidate_sublink);
                continue;
            }
            List *derived_sublinks_copy = NIL;
            for (auto derived_above : derived_sublink_vec)
            {
                derived_sublinks_copy = lappend_int(derived_sublinks_copy, derived_above);
            }
            const List *reachable_by_current = recurse_pack.derivable_map->at(candidate_sublink);
            reachable_sublinks = list_union_int(reachable_sublinks, reachable_by_current);

            const auto ref_count = get_reference_count(recurse_pack.layer_count_map, candidate_sublink);

            auto pending_entries = recurse_pack.pending_sublinks->at(original_sublink);

            // Need to check if we may ignore the sublinks.
            if (recurse_pack.sublinks_to_ignore)
            {
                const bool is_ignorable = std::find(
                                              recurse_pack.sublinks_to_ignore->begin(),
                                              recurse_pack.sublinks_to_ignore->end(),
                                              candidate_sublink) != recurse_pack.sublinks_to_ignore->end();
                if (is_ignorable)
                    continue;
            }
            else
            {
                // Need to check if the base counts are the same.
                if (eager_entry_count_map->find(candidate_sublink) == eager_entry_count_map->end())
                {
                    elog(ERROR, "Expected to the find %d in eager entry count map!", candidate_sublink);
                }
                if (expected_entry_count_map->find(candidate_sublink) == expected_entry_count_map->end())
                {
                    elog(ERROR, "Expected to find %d in the expected entry count map!");
                }
                if (eager_entry_count_map->at(candidate_sublink) != expected_entry_count_map->at(candidate_sublink))
                    continue;
            }
            // Need to check if we also need to rederive any of the prior sublinks. ugh.
            List *referring_links = get_referring_layers(recurse_pack.derivable_map, candidate_sublink);
            // Need to take the intersection with the ones that we've already derived.
            // This is also why having an over set sublinks is fine in the referring layers.
            List *rederivables = list_intersection_int(referring_links, derived_sublinks_copy);
            if (list_length(rederivables) > 1)
            {
                elog(ERROR, "Got more than 1 sublinks to rederive. This should not be possible!");
            }
            auto rederivable_head = list_head(rederivables);
            TraceProvLayerNumber higher_layer_to_redrive = 0;
            if (rederivable_head != NULL)
            {
                higher_layer_to_redrive = lfirst_int(rederivable_head);
                candidate_sublink = higher_layer_to_redrive;
                // Also set the pending entries to the higher sublink.
                pending_entries = recurse_pack.pending_sublinks->at(candidate_sublink);
            }
            elog(INFO, "Handling correlated of count: %d", traceprov_non_zero_count(pending_entries));
            auto join_condition = new TraceProvJoinConditions;
            ListCell *offset_cursor;
            foreach (offset_cursor, pending_entries)
            {
                const int offset = lfirst_int(offset_cursor);
                if (offset == 0)
                {
                    // This will happen, for example, when dealing with cases where there are disjoint paths
                    // to a sublink
                    continue;
                }
                const int other_idx = foreach_current_index(offset_cursor) + 1;
                TraceProvColumn *join_column_1 = new TraceProvColumn(1, offset);
                TraceProvColumn *join_column_2 = new TraceProvColumn(2, other_idx);
                join_condition->push_back(new TraceProvJoinPair(join_column_1, join_column_2));
            }
            TraceProvDependency *sublink_graph = tp_get_sublink_graph(parse_context, candidate_sublink);
            TraceProvNode *sublink_node = simple_read_from_log(sublink_graph, worker_local_contexts, parse_context, recurse_pack);
            // need to read all the sublink logs, perform the join, and _then_ perform the derivation.
            // This is done because, otherwise, we may do a lot of work on the right hand side ultimately doing to waste
            // IF the optimizer is not able to reorder them...
            TraceProvJoinExpr *join_exprn = make_traceprov_join_expr(
                node,
                sublink_node,
                join_condition,
                nullptr,
                true,
                true);
            const uint32 new_idx_start = previous_start_idx;
            auto child_recurse_pack = reset_entry_count_map(&recurse_pack, sublink_graph->headNumber);
            // There might be some sublinks that are, eventually, derivable by the subquery, but should NOT be derived just yet
            // because we've not finished adding all the entries to the pending, just yet.
            // To figure out those cases, we _could_ look at the derivable sublinks from the sublink and figure out those that need
            // to be filtered out. Another, simpler, way is to just look at the current's layers ref count, and just consider the ones
            // that don't have been added completely yet (whole lot more simpler)
            std::vector<TraceProvLayerNumber> sublinks_to_ignore;
            for (auto ref_count : *expected_entry_count_map)
            {
                const TraceProvLayerNumber maybe_ignorable = ref_count.first;
                if (eager_entry_count_map->find(maybe_ignorable) == eager_entry_count_map->end())
                {
                    sublinks_to_ignore.push_back(maybe_ignorable);
                    continue;
                }
                // Cannot ignore in this case.
                if (eager_entry_count_map->at(maybe_ignorable) == ref_count.second)
                    continue;
                sublinks_to_ignore.push_back(maybe_ignorable);
            }
            // Need to append the ignorable sublinks from parent too.
            if (recurse_pack.sublinks_to_ignore)
            {
                for (auto ignorable : *recurse_pack.sublinks_to_ignore)
                {
                    sublinks_to_ignore.push_back(ignorable);
                }
            }
            child_recurse_pack.sublinks_to_ignore = &sublinks_to_ignore;
            // Perform all the derivation on the join exprn now.
            auto ast_node = derive_on_node(
                (TraceProvNode *)join_exprn,
                sublink_graph,
                new_idx_start,
                (struct local_context *)NULL,
                worker_local_contexts,
                parse_context,
                child_recurse_pack);
            if (higher_layer_to_redrive != 0)
            {
                // Need to be careful in this case.
                // Since we rederived a sublink, need to only choose the one that we orginally set out to derive.
                TraceProvResultMap res_map;
                const List *directly_derivable_layers = get_directly_derivable(tp_get_sublink_graph(parse_context, original_sublink), parse_context);
                flattenTraceProvInferAbstractTree(ast_node, &res_map, parse_context);
                ListCell *derivable_layer;
                foreach (derivable_layer, directly_derivable_layers)
                {
                    const TraceProvLayerNumber derivable_layer_num = lfirst_int(derivable_layer);
                    // Here, need to, also, get rid of things that were _directly_ derivable in that sublink anyways.
                    if (res_map.find(derivable_layer_num) == res_map.end())
                    {
                        elog(INFO, "Skipping %d in ast result!", derivable_layer_num);
                    }
                    auto derived_node = res_map.at(derivable_layer_num);
                    // Basically, pretend that we didn't just derive a bunch of other things.
                    auto derived_ast_node = makeTraceProvInferAbstractTree(derivable_layer_num);
                    derived_ast_node->nodes->push_back(derived_node);
                    current_tree->children->push_back(derived_ast_node);
                }
            }
            else
            {
                current_tree->children->push_back(ast_node);
            }
            insert_duplicate_safe(recurse_pack.derived_sublinks, original_sublink);
        }

        return current_tree;
    }

    // Generic handling of derivation.
    // This doesn't care about whether node is a direct log
    // This is used for nested aggregation + sublink handling.
    static TraceProvInferAbstractTree *derive_on_node(
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
        TraceProvRecursePack recurse_pack)
    {
        auto current_tree = makeTraceProvInferAbstractTree(graph->headNumber);
        ListCell *entry_cursor;
        bool did_append_self = false;

        // Also check if any sublink are refered in the entries.
        // This is also done before handling the we process the children, so that for child sublinks we're ready to process them.
        current_tree->children->push_back(
            derive_sublinks(node, graph, parse_context, worker_local_contexts, idx_start, recurse_pack));

        foreach (entry_cursor, graph->entries)
        {
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->kind == TP_ENTRY_SET_POINTER)
            {
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
                        recurse_pack));
            }
            if (te->kind == TP_ENTRY_KIND_BASE_RELATION || (te->kind == TP_ENTRY_CORRELATION_ATTR))
            {
                if (!did_append_self)
                {
                    current_tree->nodes->push_back(node);
                    did_append_self = true;
                }
            }
            else if (te->kind == TP_ENTRY_KIND_POINTER)
            {
                if (te->is_pointer_for_window)
                    continue;
                TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(graph->children, foreach_current_index(entry_cursor));
                current_tree->children->push_back(
                    derive_aggregate_on_node(
                        node,
                        idx_start + foreach_current_index(entry_cursor) + 1,
                        child_graph,
                        current_local_context,
                        worker_local_contexts,
                        parse_context,
                        increment_recursion(&recurse_pack)));
            }
            else if (te->kind == TP_ENTRY_FRAME_START)
            {
                // The next entry should be the end.
                const TraceProvLayerNumber current_window_layer_number = te->window_entry.log_layer_number;
                const TraceProvEntry *next_te = (TraceProvEntry *)lfirst(lnext(graph->entries, entry_cursor));
                if (next_te->kind != TP_ENTRY_FRAME_END)
                {
                    elog(ERROR, "Expected the next entry to be the frame end!");
                }
                if (next_te->window_entry.log_layer_number != current_window_layer_number)
                {
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
                        recurse_pack));
            }
        }
        return current_tree;
    }

    static int find_first_set_number_entry(const List *entries, int set_number)
    {
        ListCell *entry_cursor;
        foreach (entry_cursor, entries)
        {
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->setNumber == set_number)
            {
                if (te->kind == TP_ENTRY_SET_POINTER)
                {
                    return -1;
                }
                return foreach_current_index(entry_cursor);
            }
        }
        elog(ERROR, "Didn't find the set entry!");
        return -1;
    }

    static TraceProvNode *simple_read_from_log(
        TraceProvDependency *graph,
        const std::vector<struct local_context *> *worker_local_contexts,
        TraceProvParseContext *parse_context,
        TraceProvRecursePack recurse_pack)
    {
        const TraceProvLayerNumber log_layer_number = graph->headNumber;
        const uint32 layer_width = list_length(graph->entries);
        auto found_worker_layers = find_layers_across_workers(
            log_layer_number,
            worker_local_contexts,
            layer_width);
        List *nodes = NIL;
        for (auto worker_log : *found_worker_layers)
        {
            const uint8 worker_id = worker_log.first;
            struct traceprov_aggregate_layer *agg_layer = worker_log.second;
            TraceProvData *current_layer_data = read_all_columns(
                agg_layer->layer_number,
                worker_local_contexts,
                nullptr,
                true);
            TraceProvRelation *relation = make_traceprov_relation(
                current_layer_data,
                psprintf("log_read_to_append_%s", tp_parse_get_unique_alias(parse_context)));
            relation->rel_args = make_relation_args(0, agg_layer->layer_number);
            nodes = lappend(nodes, relation);
            break;
        }
        return make_traceprov_append(nodes, false);
    }

    static bool traceprov_use_top_level_log = false;
    // Generic version of perform_derive_from_log.
    static TraceProvInferAbstractTree *perform_derive_from_log_generic(
        TraceProvDependency *log_dependency,
        const uint8 worker_count,
        TraceProvParseContext *parsed_back_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        const bool is_top_level,
        TraceProvRecursePack recurse_pack)
    {
        const TraceProvLayerNumber log_layer_number = log_dependency->headNumber;
        const uint32 layer_width = list_length(log_dependency->entries);
        auto found_worker_layers = find_layers_across_workers(
            log_layer_number,
            worker_local_contexts,
            layer_width);
        const bool log_was_parallel = found_worker_layers->size() > 1;
        TraceProvInferAbstractTree *current_tree = makeTraceProvInferAbstractTree(log_dependency->headNumber);

        ListCell *entry_cursor = NULL;
        for (auto worker_log : *found_worker_layers)
        {
            const uint8 worker_id = worker_log.first;
            struct traceprov_aggregate_layer *agg_layer = worker_log.second;
            TraceProvRecursePack recurse_pack_worker = shallow_copy_recurse_pack(&recurse_pack);
            bool has_appended_self = false;
            auto local_context = worker_local_contexts->at(worker_id - 1);
            std::vector<struct local_context *> local_context_vec = {local_context};
            TraceProvData *current_layer_data = read_all_columns(
                agg_layer->layer_number,
                &local_context_vec,
                nullptr,
                !traceprov_use_top_level_log);
            if (!traceprov_use_top_level_log)
            {
                TraceProvRelation *top_level_log_relation = make_traceprov_relation(current_layer_data, psprintf("top_level_%s", tp_parse_get_unique_alias(parsed_back_context)));
                top_level_log_relation->rel_args = make_relation_args(0, agg_layer->layer_number);
                current_tree->children->push_back(
                    derive_on_node(
                        (TraceProvNode *)top_level_log_relation,
                        log_dependency,
                        0,
                        local_context,
                        worker_local_contexts,
                        parsed_back_context,
                        recurse_pack_worker));
                break;
            }
        }
        return current_tree;
    }

#ifdef TRACEPROV_BUILD_WITH_DUCKDB
    const bool use_duckdb = true;
#else
    const bool use_duckdb = false;
#endif

    static TraceProvResultMap *get_generic_derivation_spec(
        TraceProvParseContext **p_parsed_back_context,
        TraceProvInferSetupExtra **p_extra,
        List **out_graphs = NULL)
    {
        TraceProvParseContext *parsed_back_context = NULL;
        List *graphs = deserializeTraceProvDependency(&parsed_back_context, NULL);
        if (p_parsed_back_context)
            *p_parsed_back_context = parsed_back_context;
        if (out_graphs)
        {
            *out_graphs = graphs;
        }
        struct traceprov_shared_context shared_context;
        if (map_traceprov_shared_context(&shared_context))
        {
            elog(ERROR, "Error mmaping shared context");
        }
        const uint8 worker_count = shared_context.worker_count;
        ListCell *graph_cursor;

        auto worker_local_contexts = traceprov_get_local_contexts(worker_count);
        TraceProvLayerEntryMap layer_entry_count_map;
        TraceProvDerivableSublinkMap derivable_entry_map;
        TraceProvLastRef last_ref;
        get_used_sublinks(graphs, parsed_back_context, &layer_entry_count_map, &derivable_entry_map, &last_ref);
        get_used_sublinks(parsed_back_context->properties->sublink_map, parsed_back_context, &layer_entry_count_map, &derivable_entry_map, &last_ref);
        auto extended_derivable_map = extend_reachable(&derivable_entry_map);

        List *get_all_used_sublinks = NIL;
        for (auto res_pair : *extended_derivable_map)
        {
            get_all_used_sublinks = list_union_int(get_all_used_sublinks, res_pair.second);
        }
        auto top_tree = makeTraceProvInferAbstractTree(0);
        auto size_layer_map = new TraceProvSizeLayers;
        auto setup_extra = new TraceProvInferSetupExtra;
        setup_extra->size_layer_map = size_layer_map;
        setup_extra->worker_count = worker_count;
        if (p_extra)
            *p_extra = setup_extra;
        TraceProvLayerEntriesMap layer_entries_map;
        foreach (graph_cursor, graphs)
        {
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
                    TraceProvRecursePack{
                        .depth_map = NULL,
                        .level = 0,
                        .pending_sublinks = new TraceProvPendingSublinks,
                        .size_layer_map = size_layer_map,
                        .layer_count_map = &layer_entry_count_map,
                        .derivable_map = extended_derivable_map,
                        .base_derivable_map = &derivable_entry_map,
                        .current_layer_number = graph->headNumber,
                        .derived_sublinks = new std::vector<TraceProvLayerNumber>,
                        .entry_count_map = new TraceProvEntryCountMap,
                        .last_ref = &last_ref,
                        .sublinks_to_ignore = nullptr,
                        .combine_main_worker = 0}));
            create_layer_entry_map(graph, &layer_entries_map);
        }
        ListCell *sublink_cursor;
        foreach (sublink_cursor, parsed_back_context->properties->sublink_map)
        {
            TraceProvDependency *child_sublink = (TraceProvDependency *)lfirst(sublink_cursor);
            create_layer_entry_map(child_sublink, &layer_entries_map);
            // If a sublink is being used, don't derive it.
            // It should be automatically be derived as part of generic handling.
            if (traceprov_find_int_list(get_all_used_sublinks, child_sublink->headNumber))
                continue;
            if (child_sublink->graph_type != TraceProvGraphKind::TP_LOG)
                elog(ERROR, "Expected the top level graph to always be a TP_LOG. Got %d", child_sublink->graph_type);

            top_tree->children->push_back(
                perform_derive_from_log_generic(
                    child_sublink,
                    worker_count,
                    parsed_back_context,
                    worker_local_contexts,
                    true,
                    TraceProvRecursePack{
                        .depth_map = NULL,
                        .level = 0,
                        .pending_sublinks = new TraceProvPendingSublinks,
                        .size_layer_map = size_layer_map,
                        .layer_count_map = &layer_entry_count_map,
                        .derivable_map = extended_derivable_map,
                        .base_derivable_map = &derivable_entry_map,
                        .current_layer_number = child_sublink->headNumber,
                        .derived_sublinks = new std::vector<TraceProvLayerNumber>,
                        .entry_count_map = new TraceProvEntryCountMap,
                        .last_ref = &last_ref,
                        .sublinks_to_ignore = nullptr,
                        .combine_main_worker = 0}));
        }

        auto result_map = new TraceProvResultMap;
        // Instead of making just one result map, we make smaller ones.
        // Then, we look for intersections against previously derived ones.
        // Any common ones are assumed to be sublinks (assertion), and then "joined" over.
        for (auto child_tree : *top_tree->children)
        {
            auto child_result_map = new TraceProvResultMap;
            flattenTraceProvInferAbstractTree(child_tree, child_result_map, parsed_back_context);
            for (auto child_result_map_entry : *child_result_map)
            {
                const TraceProvLayerNumber child_layer_num = child_result_map_entry.first;
                if (result_map->find(child_layer_num) == result_map->end())
                {
                    result_map->insert({child_layer_num, child_result_map_entry.second});
                    continue;
                }
                // In this case, we found the layer previously derived.
                // Transform that into an join. Before we do that, need to determine what kind of node we're dealing with.
                // If it is a sublink, with correlated attrs, also need to "ignore" them in the joined node.
                const List *entries = layer_entries_map.at(child_layer_num);
                const uint32 correlated_attr_count = count_correlated_attrs(entries);
                TraceProvNode *original_node = result_map->at(child_layer_num);
                TraceProvNode *current_node = child_result_map_entry.second;
                const uint64 original_node_column_count = traceprov_get_node_column_count(original_node);
                const uint64 current_node_column_count = traceprov_get_node_column_count(current_node);
                if (original_node_column_count < correlated_attr_count)
                {
                    elog(ERROR, "Expected the (original) column count to at least be correlated attribute count!");
                }
                if (current_node_column_count < correlated_attr_count)
                {
                    elog(ERROR, "Expected the (current) column count to at least be correlated attribute count!");
                }
                auto join_condition = new TraceProvJoinConditions;
                auto output_cols = new std::vector<TraceProvColumn *>;
                for (uint32_t idx = 0; idx < original_node_column_count - correlated_attr_count; idx++)
                {
                    output_cols->push_back(new TraceProvColumn(1, idx + 1));
                }
                // This does all the way (including the correlated attr count)
                for (uint32_t idx = 0; idx < current_node_column_count; idx++)
                {
                    output_cols->push_back(new TraceProvColumn(2, idx + 1));
                }
                for (uint32_t idx = 0; idx < correlated_attr_count; idx++)
                {
                    auto left_column = new TraceProvColumn(1, (original_node_column_count - correlated_attr_count + idx + 1));
                    auto right_column = new TraceProvColumn(2, (current_node_column_count - correlated_attr_count + idx + 1));
                    join_condition->push_back(new TraceProvJoinPair(left_column, right_column));
                }
                auto join_expr = make_traceprov_join_expr(
                    original_node,
                    current_node,
                    join_condition,
                    output_cols);
                result_map->at(child_layer_num) = (TraceProvNode *)join_expr;
            }
        }
        // Go over the result maps and check if we need to add sequential scans.
        for (auto result_map_pair : *result_map)
        {
            if (result_map_pair.second->tag == T_TP_RELATION)
            {
                // In this case, it is a simple scan.
                // Apparently, for some reason, DuckDB does not parallelise this????
                // Anyways, right now, that breaks things.
                // So, remember that it was a simple scan.
                TraceProvRelation *relation = (TraceProvRelation *)result_map_pair.second;
                relation->rel_args->table_flags |= TRACEPROV_TABLE_SEQ_SCAN;
            }
        }
        return result_map;
    }

    PG_FUNCTION_INFO_V1(traceprov_perform_generic_derivation);

    Datum traceprov_perform_generic_derivation(PG_FUNCTION_ARGS)
    {

        TraceProvLayerNumber result_to_return = PG_GETARG_INT64(0);
        TraceProvParseContext *parsed_back_context = NULL;
        TraceProvInferSetupExtra *setup_extra = NULL;
        auto result_map = get_generic_derivation_spec(&parsed_back_context, &setup_extra);

        if (traceprov_current.infer_context == NULL)
        {
            traceprov_duckdb_setup_context(&traceprov_current.infer_context, &traceprov_current.cleanup_infer_context, setup_extra);
        }

        auto derived_node_map = new std::unordered_map<TraceProvLayerNumber, TraceProvTopResult *>;
        for (auto child : *result_map)
        {
            elog(INFO, "Node Idx: %d", child.first);
            TraceProvNode *node = child.second;
            const char *sql = traceprov_node_to_sql(node, TraceProvToSQLContext{.context = parsed_back_context, .use_table_def = true});
            TraceProvData *data = nullptr;
            uint64 duration = 0;
            if (use_duckdb)
            {
                elog(INFO, "Used duckdb!");
                {
                    TP_EVALUATE_START();
                    data = traceprov_perform_duckdb_inference(sql, traceprov_current.infer_context);
                    TP_EVALUATE_END();
                    duration = TP_EVALUATE_DURATION();
                }
            }
            else
            {
                TraceProvEvaluateNodeContext *eval_context = new TraceProvEvaluateNodeContext;
                eval_context->should_dump = false;
                elog(ERROR, "Only supporting duckdb now!");
            }

            elog(INFO, "Gen SQL: %s", sql);
            elog(INFO, "Size %ld", data->at(0)->data->size());
            elog(INFO, "Width %ld", data->size());
            elog(INFO, "Time %ld", duration);
            auto top_result = new TraceProvTopResult;
            top_result->pdata = list_make1(data);
            top_result->width = data->size();
            derived_node_map->insert({child.first, top_result});
        }
        if (derived_node_map->find(result_to_return) == derived_node_map->end())
            elog(ERROR, "Didn't find the input result idx!");
        TraceProvTopResult *selected_result = derived_node_map->at(result_to_return);
        traceprov_materialize_derived_result(fcinfo, selected_result);
        return (Datum)0;
    }

    static TraceProvInferResult last_result;
    // Same set up as traceprov_perform_generic_derivation.
    // But is faster :)
    // Accomplished by directly copy data from DuckDB to Postgres without storing it.
    PG_FUNCTION_INFO_V1(traceprov_perform_duckdb_inference_fast);

    Datum traceprov_perform_duckdb_inference_fast(PG_FUNCTION_ARGS)
    {
        const TraceProvLayerNumber result_to_return = PG_GETARG_INT64(0);
        const bool is_dry_infer = PG_NARGS() > 1 ? PG_GETARG_BOOL(1) : false;
        TraceProvParseContext *parsed_back_context = NULL;
        TraceProvInferSetupExtra *setup_extra;
        auto result_map = get_generic_derivation_spec(&parsed_back_context, &setup_extra);

        if (traceprov_current.infer_context == NULL)
        {
            traceprov_duckdb_setup_context(&traceprov_current.infer_context, &traceprov_current.cleanup_infer_context, setup_extra);
        }

        if (result_map->find(result_to_return) == result_map->end())
        {
            elog(ERROR, "Expected to find the node!!");
        }

        TraceProvNode *node_to_eval = result_map->at(result_to_return);
        const char *sql = traceprov_node_to_sql(node_to_eval, TraceProvToSQLContext{.context = parsed_back_context, .use_table_def = true});
        const uint32 expected_column_width = traceprov_get_node_column_count(node_to_eval);
        if (!is_dry_infer)
            traceprov_prepare_for_materialize(fcinfo, expected_column_width);
        TraceProvInferResult result = traceprov_perform_duckdb_inference_pg_copy(sql, traceprov_current.infer_context, fcinfo, expected_column_width);
        last_result = result;
        return (Datum)0;
    }

    PG_FUNCTION_INFO_V1(traceprov_get_infer_stat);

    Datum traceprov_get_infer_stat(PG_FUNCTION_ARGS)
    {
        const bool perform_infer = PG_GETARG_BOOL(1);
        // This allows this function be reused directly after the prior call.
        if (perform_infer)
        {
            traceprov_perform_duckdb_inference_fast(fcinfo);
        }

        // Convert the infer result to json.
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '{');
        appendStringInfo(&buf, "\"width\": %ld", last_result.width);
        appendStringInfoChar(&buf, ',');
        appendStringInfo(&buf, "\"time\": %ld", last_result.time);
        appendStringInfoChar(&buf, ',');
        appendStringInfo(&buf, "\"row_count\": %ld", last_result.row_count);
        appendStringInfoChar(&buf, '}');
        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }

    PG_FUNCTION_INFO_V1(traceprov_run_duckdb_query);

    // Does exactly the same setup as generic setup, but allows running generic
    // input queries. SQL injection go brrrr.
    Datum traceprov_run_duckdb_query(PG_FUNCTION_ARGS)
    {

        char *sql_to_run = PG_GETARG_CSTRING(0);
        TraceProvParseContext *parsed_back_context = NULL;
        TraceProvInferSetupExtra *setup_extra = NULL;
        (void)get_generic_derivation_spec(&parsed_back_context, &setup_extra);

        if (traceprov_current.infer_context == NULL)
        {
            traceprov_duckdb_setup_context(&traceprov_current.infer_context, &traceprov_current.cleanup_infer_context, setup_extra);
        }
        TraceProvData *data = traceprov_perform_duckdb_inference(sql_to_run, traceprov_current.infer_context);
        elog(INFO, "Gen SQL: %s", sql_to_run);
        elog(INFO, "Size %ld", data->at(0)->data->size());
        elog(INFO, "Width %ld", data->size());
        auto top_result = TraceProvTopResult{
            .pdata = list_make1(data),
            .width = data->size()};
        traceprov_materialize_derived_result(fcinfo, &top_result);
        return (Datum)0;
    }

    PG_FUNCTION_INFO_V1(traceprov_get_generic_derivation_spec);

    Datum traceprov_get_generic_derivation_spec(PG_FUNCTION_ARGS)
    {

        const bool perform_execution = PG_GETARG_BOOL(0);

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping shared context");
        }

        TraceProvParseContext *parse_context = NULL;
        TraceProvInferSetupExtra *setup_extra = NULL;
        auto result_map = get_generic_derivation_spec(&parse_context, &setup_extra);
        if (traceprov_current.infer_context == NULL)
        {
            traceprov_duckdb_setup_context(&traceprov_current.infer_context, &traceprov_current.cleanup_infer_context, setup_extra);
        }
        std::string graph_str = "{";
        graph_str.append("\"elements\": [");
        bool needs_sep = false;

        for (auto child : *result_map)
        {
            if (needs_sep)
                graph_str.append(",");
            const TraceProvLayerNumber idx = child.first;
            TraceProvNode *node = child.second;
            TraceProvParseContext temp_sql_context;
            tp_parse_initialize_context(&temp_sql_context);
            temp_sql_context.root_context = &temp_sql_context;
            // This is done like this so that the numbering of nodes is always consistent.
            const char *sql = traceprov_node_to_sql(node, TraceProvToSQLContext{.context = &temp_sql_context, .use_table_def = true});
            TraceProvData *data = nullptr;
            if (perform_execution)
            {
                data = traceprov_perform_duckdb_inference(sql, traceprov_current.infer_context);
            }
            needs_sep = true;
            graph_str.append("{");
            graph_str.append("\"idx\": ");
            graph_str.append(std::to_string(idx));
            graph_str.append(",");
            graph_str.append("\"sql\": ");
            graph_str.append("\"");
            graph_str.append(sql);
            graph_str.append("\"");
            if (perform_execution)
            {
                graph_str.append(",");
                graph_str.append("\"width\": " + std::to_string(data->size()));
                graph_str.append(",");
                graph_str.append("\"rows\": " + std::to_string(data->at(0)->data->size()));
            }
            else
            {
                graph_str.append(",");
                graph_str.append("\"expected_width\": " + std::to_string(traceprov_get_node_column_count(node)));
            }
            graph_str.append("}");
        }
        graph_str.append("]");
        graph_str.append(",");
        graph_str.append("\"min_local_used\": ");
        graph_str.append(std::to_string(context.maximum_layer_number_used));
        graph_str.append("}");
        PG_RETURN_TEXT_P(cstring_to_text(graph_str.c_str()));
    }

    // Prepares for scan, and returns the create table commands to make simpler (and automated)
    PG_FUNCTION_INFO_V1(traceprov_prepare_for_scan);

    Datum traceprov_prepare_for_scan(PG_FUNCTION_ARGS)
    {
        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping shared context");
        }

        TraceProvParseContext *parse_context = NULL;
        TraceProvInferSetupExtra *setup_extra = NULL;
        List *out_graphs = NIL;
        auto result_map = get_generic_derivation_spec(&parse_context, &setup_extra, &out_graphs);
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '{');
        auto scan_map = new std::unordered_map<TraceProvLayerNumber, TraceProvRelationInferExtraItem>;
        // appendStringInfo(&buf, "\"width\": %ld", last_result.width);
        // appendStringInfoChar(&buf, ',');
        // appendStringInfo(&buf, "\"time\": %ld", last_result.time);
        // appendStringInfoChar(&buf, ',');
        // appendStringInfo(&buf, "\"row_count\": %ld", last_result.row_count);
        bool needs_sep = false;
        appendStringInfo(&buf, "\"sql\": [");
        for (auto child : *result_map)
        {
            const TraceProvLayerNumber idx = child.first;
            TraceProvNode *node = child.second;
            const char *sql = traceprov_node_to_sql(node, TraceProvToSQLContext{.context = parse_context, .use_table_def = true});
            const auto col_count = traceprov_get_node_column_count(node);
            scan_map->insert({idx, TraceProvRelationInferExtraItem{.sql = (new std::string(sql))->c_str(), .expected_col_width = col_count}});
            StringInfoData create_table_buff;
            initStringInfo(&create_table_buff);
            auto table_name = psprintf(TRACEPROV_RELATION_INFER_NAME, idx);
            StringInfoData col_buff;
            initStringInfo(&col_buff);
            for (uint64_t col_idx = 0; col_idx < col_count; col_idx++)
            {
                if (col_idx > 0)
                {
                    appendStringInfoChar(&col_buff, ',');
                }
                appendStringInfo(&col_buff, "%s BIGINT", psprintf("col_%ld", col_idx));
            }
            appendStringInfo(&create_table_buff, "\"CREATE TABLE %s(%s) USING traceprov_am;\"", table_name, col_buff.data);
            if (needs_sep)
            {
                appendStringInfoChar(&buf, ',');
            }
            needs_sep = true;
            appendStringInfoString(&buf, create_table_buff.data);
        }
        if (g_tp_relation_infer_extra.map)
        {
            delete g_tp_relation_infer_extra.map;
        }
        if (g_tp_relation_infer_extra.log_offset)
        {
            delete g_tp_relation_infer_extra.log_offset;
        }
        g_tp_relation_infer_extra.map = scan_map;
        g_tp_relation_infer_extra.log_offset = nullptr;
        if (PG_NARGS() == 1)
        {
            // Need to look at the offset.
            const int64 offset = PG_GETARG_INT64(0);
            const TraceProvDependency *graph = (TraceProvDependency *)list_nth(out_graphs, 0);
            if (offset >= 0)
            {
                const TraceProvLayerNumber top_layer = graph->headNumber;
                auto log_offset_map = new TraceProvLogOffsetMap;
                log_offset_map->insert({top_layer, offset});
                g_tp_relation_infer_extra.log_offset = log_offset_map;
            }
        }
        appendStringInfoChar(&buf, ']');
        appendStringInfoChar(&buf, '}');
        if (traceprov_current.infer_context == NULL)
        {
            traceprov_duckdb_setup_context(&traceprov_current.infer_context, &traceprov_current.cleanup_infer_context, setup_extra);
        }
        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }

    PG_FUNCTION_INFO_V1(traceprov_get_total_layer_size);

    Datum traceprov_get_total_layer_size(PG_FUNCTION_ARGS)
    {

        struct traceprov_shared_context context;

        if (map_traceprov_shared_context(&context))
        {
            elog(ERROR, "Error mmaping the shared context");
        }

        auto local_contexts = traceprov_get_local_contexts(context.worker_count);

        TraceProvLogSize log_size = {
            .page_requested_size = 0,
            .page_used_size = 0,
            .bytes_used_size = 0};
        uint64_t page_requested = 0;
        uint64_t page_used = 0;
        uint64_t bytes_used = 0;

        for (auto local_context : *local_contexts)
        {
            for (idx_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++)
            {
                const traceprov_aggregate_layer *layer = &local_context->cached_layers[layer_idx];
                if (layer->layer_number == 0)
                    continue;
                page_requested += layer->size;
                page_used += layer->initial_allocation_size;
                const uint64_t gap = ((uint64_t)layer->current_row - (uint64_t)layer->last_mapping);
                bytes_used += gap;
                if (layer->size > layer->initial_allocation_size)
                {
                    // Need to determine which pages were used.
                    bytes_used += TRACEPROV_PAGE_SIZE * (layer->initial_allocation_size); // for the first page.
                    const int64_t allocate_count = (layer->size - layer->initial_allocation_size) / TRACEPROV_INCREMENT_TRACE_BY_PG;
                    if (allocate_count <= 0)
                    {
                        elog(ERROR, "Expected allocations to be > 1");
                    }
                    const uint64_t intermediate_page_used = (allocate_count - 1) * TRACEPROV_INCREMENT_TRACE_BY_PG;
                    page_used += intermediate_page_used;
                    bytes_used += (intermediate_page_used * TRACEPROV_PAGE_SIZE);
                    if (gap == 0)
                    {
                        // This can never happen.
                        // Otherwise, it will imply that the increment go through, but no nemory access was performed, which is not possible.
                        elog(ERROR, "Should never expect gap to be 0");
                    }
                    page_used += ((gap - 1) / TRACEPROV_PAGE_SIZE) + 1;
                }
            }
            traceprov_fail_safe_unmap(local_context, sizeof(struct local_context));
        }
        log_size.page_requested_size = page_requested * TRACEPROV_PAGE_SIZE;
        log_size.page_used_size = page_used * TRACEPROV_PAGE_SIZE;
        log_size.bytes_used_size = bytes_used;

        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '{');
        appendStringInfo(&buf, "\"page_requested_size\": %ld", log_size.page_requested_size);
        appendStringInfoChar(&buf, ',');
        appendStringInfo(&buf, "\"page_used_size\": %ld", log_size.page_used_size);
        appendStringInfoChar(&buf, ',');
        appendStringInfo(&buf, "\"bytes_used_size\": %ld", log_size.bytes_used_size);
        appendStringInfoChar(&buf, '}');

        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }
};
