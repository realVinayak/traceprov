// Like traceprov's normal infer, but returns the set of all rows inline for postgres
// rather than dumping logic.
// Currently, only handles simple layers.
// TODO: Migrate all the arguments.
// TODO: Migrate polynomials generation in some other file?

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>
#include <fstream>

#undef HAVE__BUILTIN_TYPES_COMPATIBLE_P

extern "C" {
    #include "postgres.h"
    #include "funcapi.h"
    #include "fmgr.h"
    #include "miscadmin.h"
    #include "traceprov.h"
    #include "file_utils.h"
    #include "utils/builtins.h"
    #include "traceprov_parse_context.h"
    #include "rewriter_utils.h"
    #include "traceprov_infer.hpp"
    PG_MODULE_MAGIC;
}

extern "C" {
    // static_assert(!(HAVE__BUILTIN_TYPES_COMPATIBLE_P))

    #define UNUSED(X) do {} while(0 && X);

    static TraceProvData* traceprov_evaluate_join_exprn(TraceProvJoinExpr *join_exprn, TraceProvEvaluateNodeContext *eval_context);
    static TraceProvData *traceprov_evaluate_node(TraceProvNode *node, TraceProvEvaluateNodeContext *eval_context);
    static TraceProvData *traceprov_evaluate_append(TraceProvAppend *append_node, TraceProvEvaluateNodeContext *eval_context);
    static void traceprov_dump_data_to_csv(TraceProvData *data, char *file_name, bool include_headers = false);
    uint64 traceprov_get_node_column_count(TraceProvNode *node);
    // Converts the node to SQL.
    static char *traceprov_node_to_sql(TraceProvNode *node, TraceProvParseContext *context);

    static TraceProvData *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const struct local_context *local_context,
        const TraceProvDependency *dependency
    );

    static TraceProvColumnData *traceprov_make_empty_column();
    static void traceprov_materialize_derived_result(FunctionCallInfo fcinfo, TraceProvData *derived);

    static TraceProvDerivedNode *traceprov_make_derived_node(TraceProvLayerNumber layer_number, TraceProvNode *node){
        TraceProvDerivedNode *derived = (TraceProvDerivedNode *)malloc(sizeof(TraceProvDerivedNode));
        derived->layer_number = layer_number;
        derived->node = node;
        return derived;
    }

    static TraceProvData *traceprov_make_column_data(
        std::vector<std::vector<uint64>*> *vectors
    ){
        auto data = palloc0_object(TraceProvData);
        for (auto column: *vectors){
            auto column_repr = traceprov_make_empty_column();
            column_repr->data = column;
            data->push_back(column_repr);
        }
        return data;
    }

    static TraceProvColumnData *traceprov_make_empty_column(){
        auto col_data = palloc0_object(TraceProvColumnData);
        col_data->data = new std::vector<uint64>;
        return col_data;
    }
    

    static std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32 worker_count){
        auto worker_local_contexts = new std::vector<struct local_context *>;
        for (uint8 worker_id = 0; worker_id < worker_count; worker_id++){
            int fd = open(psprintf(TRACEPROV_WORKER_LAYER_MAP, DataDir, worker_id + 1), O_RDONLY);
            if (fd < 0) elog(ERROR, "Error opening the worker laye rmap!");
            void *ptr = mmap(
                NULL,
                sizeof(struct local_context),
                PROT_READ,
                MAP_SHARED,
                fd,
                0
            );
            if (ptr == MAP_FAILED){
                elog(ERROR, "Error mmaping the layer file!");
            }
            struct local_context *worker_local_context = (struct local_context *)ptr;
            worker_local_contexts->push_back(worker_local_context);
        }
        return worker_local_contexts;
    }

    int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size){
        if (unlikely(worker_id == 0)){
            elog(ERROR, "Didn't expect to ever be called for worker_id == 0");
        }
        char *file_name = get_bi_injected_str(TRACEPROV_MAIN_TRACE_FILE, DataDir, layer_number, worker_id, NULL);
        if (file_name == NULL) return 1;
        int fd = open(file_name, O_RDONLY);
        if (fd < 0) {
            PRINT_ON_DEBUG("Error opening the group layer file");
            return 1;
        }
        void *temp_ptr = mmap(
            NULL,
            file_size * TRACEPROV_PAGE_SIZE,
            PROT_READ,
            MAP_SHARED,
            fd,
            0
        );

        if (temp_ptr == MAP_FAILED){
            PRINT_ON_DEBUG("Error mmaping group layer file");
            return 1;
        }
        close(fd);
        *ptr = temp_ptr;
        return 0;
    }

    int map_traceprov_shared_context(struct traceprov_shared_context *ptr){
        const size_t size_shared_context_filename = sizeof(TRACEPROV_SHARED_CONTEXT) + strlen(DataDir) + 1;
        int rc = 0;
        struct traceprov_shared_context *temp_ptr;
        char *shared_context_filename = (char*)malloc(size_shared_context_filename);
        if (shared_context_filename == NULL){
            elog(ERROR, "Couldn't allocate memory to hold shared context file");
            return 1;
        }
        memset(shared_context_filename, 0, size_shared_context_filename);
        sprintf(shared_context_filename, TRACEPROV_SHARED_CONTEXT, DataDir);

        int shared_context_fd = open(shared_context_filename, O_RDONLY);
        if (shared_context_fd < 0){
            PRINT_ON_DEBUG("Error opening the scratch file");
            goto exit_map;
        }

        temp_ptr = (struct traceprov_shared_context *)mmap(
            NULL,
            TRACEPROV_SHARED_CONTEXT_SIZE,
            PROT_READ,
            MAP_SHARED,
            shared_context_fd,
            0
        );

        if (temp_ptr == MAP_FAILED){
            PRINT_ON_DEBUG("Error mapping the scratch file");
            goto exit_map;
        }

        PRINT_ON_DEBUG("Map shared context succesful!");

        memcpy(ptr, temp_ptr, sizeof(struct traceprov_shared_context));

    exit_map:
        if (shared_context_fd > 0) close(shared_context_fd);
        if (shared_context_filename) free(shared_context_filename);
        return rc;
    }

    void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer){
        const uint64 gap = ((uint64)layer->current_row - (uint64)layer->last_mapping);
        assert(gap >= 0);
        // Now, figure out what the last mapped region will have been (or the starting address of it.)
        const uint64 infered_gap = layer->size == 1 ? 0 : (layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
        void *final_row = (void*)((uint64)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);
        return final_row;
    }

    struct infer_result {
        size_t width;
        std::vector<uint64> **ids;
    };

    void store_inference(
        Tuplestorestate *tupstore,
        TupleDesc tupdesc,
        const struct infer_result *result
    ){
        const size_t width = result->width;
        std::vector<uint64> ** pk_records = result->ids;
        bool * nulls = (bool *)malloc(sizeof(bool)*width);
        memset(nulls, 0, sizeof(bool)*width);

        Datum *records = (Datum *)malloc(sizeof(Datum)*width);

        for (size_t record_index = 0; record_index < pk_records[0]->size(); record_index++){
            for (size_t key_index = 0; key_index < width; key_index++){
                records[key_index] = Int64GetDatumFast(pk_records[key_index]->at(record_index));
            }

            tuplestore_putvalues(tupstore, tupdesc, records, nulls);
        }

        #if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
        #endif
        return;
    }

    std::vector<uint64> *set_diff(std::vector<uint64> *first, std::vector<uint64> *second){
      // set diff, assumes sorted.
      unsigned long int iter_first = 0;
      unsigned long int iter_second = 0;
      std::vector<uint64> *set_diff_computed = new std::vector<uint64>;
      while (iter_first < first->size()){
              bool did_loop = false;
              while((iter_second < second->size()) && (first->at(iter_first) == second->at(iter_second))) {
                did_loop = true;
                iter_second++;
              }
              if (did_loop) { iter_first++; continue;}
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
        uint8 worker_id
    ){
        // We need to get the recorded input in the layer file (given the reference)
        // It is possible that we used parallel (omitted for now.)
        const TraceProvLayerNumber layerNumber = graph->headNumber;
        const struct local_context *context = NULL;
        const struct traceprov_aggregate_layer *layer = &context->cached_layers[graph->headNumber - 1];

        std::sort(reference->begin(), reference->end());

        void *ptr_layer_row = NULL;
        if (map_layer_file(layerNumber, worker_id, &ptr_layer_row, layer->size)){
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        const void *ptr_final_row = get_final_ptr(ptr_layer_row, layer);
        const int entryCount = list_length(graph->entries);

        std::vector<std::vector<uint64>*> *entriesValues = new std::vector<std::vector<uint64>*>;
        for (int i = 0; i < entryCount; i++){
            entriesValues->push_back(new std::vector<uint64>);
        }

        if ( layer->num_pk_records != (uint32)list_length(graph->entries)){
            elog(ERROR, "expected to log the count of entries");
        }

        while (ptr_layer_row < ptr_final_row){
            ptr_layer_row = (void *)(((uint64)layer->record_padding) + (uint64)ptr_layer_row);
            bool found = false;
            found = std::binary_search(reference->begin(), reference->end(), ((struct trace_file_forward_row*)ptr_layer_row)->group_count);
            if (found){
                for (uint32 key_idx = 0; key_idx < layer->num_pk_records; key_idx++){
                    const int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row*)ptr_layer_row), key_idx);
                    entriesValues->at(key_idx)->push_back(record_key);
                }
            }
            
            ptr_layer_row = (void*)GET_PK_FROM_ROW(((struct trace_file_forward_row*)ptr_layer_row), layer->num_pk_records);
        }

        for (int i = 0, pointerChildIdx=0; i < entryCount; i++){
            elog(INFO, "Computed lengths: %ld", entriesValues->at(i)->size());
            const TraceProvEntry *tpEntry = (TraceProvEntry *)list_nth(graph->entries, i);
            // If it is a pointer, also need to keep recursing down to the children.
            if (tpEntry->kind == TraceProvEntryKind::TP_ENTRY_KIND_POINTER){
                perform_inference_graph_worker(
                    sharedContext,
                    (TraceProvDependency *)list_nth(graph->children, pointerChildIdx),
                    entriesValues->at(i),
                    worker_id
                );
                pointerChildIdx++;
            }
        }
    }

    void perform_inference_driver(){
        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context)){
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
        if (map_layer_file(group_layer_number, context.main_worker_id, &group_layer_ptr, group_layer->size)){
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        for (uint64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++){
            const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
            if (gr->in_result){
                top_level_values->push_back(group_idx+1);
            }
        }

        perform_inference_graph_worker(&context, graph, top_level_values, 0);
    }

    struct infer_result * perform_inference(
        const unsigned int layer_number,
        const unsigned int reference_layer, 
        const unsigned int subq_layer_number
        ){

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context)){
            elog(ERROR, "Error mmaping shared context");
        }


        const int group_layer_number = layer_number + 1;
        const int partial_group_ln = layer_number + 2;

        const struct local_context *main_worker_context = NULL;
        const struct traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[layer_number - 1];
        const struct traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];
        const struct traceprov_aggregate_layer *partial_group_layer = &main_worker_context->cached_layers[partial_group_ln - 1];

        auto present_groups = new std::vector<uint64>;
        if (subq_layer_number){
            std::vector<uint64> ** subq_records = NULL;
            int subq_width = 0;
            // Here, it is entirely possible that the subquery gets parallelized.
            // So, we'd have to look at all the workers.
            const int subq_index = subq_layer_number - 1;
            for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

                const struct local_context *bg_context = NULL;
                const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[subq_index];

                if (bg_trace_layer->layer_number != subq_layer_number) continue;
                
                subq_width = bg_trace_layer->num_pk_records + 1;
                if (subq_records == NULL){
                    // Need to have +1 because of the adjusting that was done during tracing.
                    subq_records = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64> *)*(bg_trace_layer->num_pk_records + 1));
                    for (uint32 pk_id = 0; pk_id < bg_trace_layer->num_pk_records + 1; pk_id++)
                        subq_records[pk_id] = new std::vector<uint64>;
                }

                void *subq_forward_row = NULL;
                if (map_layer_file(subq_layer_number, worker_id, &subq_forward_row, bg_trace_layer->size)){
                    PRINT_ON_DEBUG("Error opening the subq trace file");
                    continue;
                }

                const void *subq_final_row = get_final_ptr(subq_forward_row, bg_trace_layer);

                while (subq_forward_row < subq_final_row){
                    subq_forward_row = (void*)((uint64)bg_trace_layer->record_padding + (uint64)subq_forward_row);
                    int64 *subq_forward_row_record = (int64*)subq_forward_row;
                    for (uint32 key_idx = 0; key_idx < bg_trace_layer->num_pk_records + 1; key_idx++, subq_forward_row_record++){
                        subq_records[key_idx]->push_back(*subq_forward_row_record);
                    }
                    subq_forward_row = (void*)subq_forward_row_record;
                }
            }

            struct infer_result *infer_result_computed = (struct infer_result*)malloc(sizeof(struct infer_result));
            
            infer_result_computed->ids = subq_records;
            infer_result_computed->width = subq_width;
            return infer_result_computed;

        }
        

        // Need to mmap the group layer file now. We're going to go mmap the entire file (because we can't have data
        // past the file contents).

        void *group_layer_ptr;
        void *forward_row;
        void *partial_group_row = NULL;

        if (map_layer_file(group_layer_number, context.main_worker_id, &group_layer_ptr, group_layer->size)){
            PRINT_ON_DEBUG("Error opening group layer file");
            elog(ERROR, "Error opening group layer file");
        }

        std::vector<uint64> * groups_to_filter = new std::vector<uint64>;
        
        if (reference_layer){
            const struct traceprov_aggregate_layer *group_reference_layer = &main_worker_context->cached_layers[reference_layer];
            void *group_reference_ptr = NULL;
            if (map_layer_file(reference_layer + 1, context.main_worker_id, &group_reference_ptr, group_reference_layer->size)){
                PRINT_ON_DEBUG("Error opening group reference layer");
                elog(ERROR, "Error opening group reference layer");
            }
            for (uint64 reference_group_idx = 0; reference_group_idx < group_layer->num_groups; reference_group_idx++){
                const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_reference_ptr)[reference_group_idx];
                if (gr->in_result){
                    groups_to_filter->push_back(reference_group_idx + 1);
                }
            }

            std::sort(groups_to_filter->begin(), groups_to_filter->end());
        }

        for (uint64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++){
            const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_layer_ptr)[group_idx];
            int should_add = false;
            if (reference_layer){
                should_add = std::binary_search(groups_to_filter->begin(), groups_to_filter->end(), gr->in_result);
            }else{
                should_add = gr->in_result;
            }
            if (should_add){
                present_groups->push_back(group_idx + 1);
            }
        }

        if (map_layer_file(layer_number, context.main_worker_id, &forward_row, main_trace_layer->size)){
            PRINT_ON_DEBUG("Error opening main trace file");
            elog(ERROR, "Error opening main trace file");
        }

        if (0 == access(
            get_bi_injected_str(TRACEPROV_MAIN_TRACE_FILE, DataDir, partial_group_ln, main_worker_context->worker_id, NULL),
            F_OK
        )){
            if (map_layer_file(partial_group_ln, context.main_worker_id, &partial_group_row, partial_group_layer->size)){
                PRINT_ON_DEBUG("Error opening the partial trace file");
            }
        }

        if (partial_group_row) PRINT_ON_DEBUG("Using partial trace file");

        std::sort(present_groups->begin(), present_groups->end());
        
        std::vector<uint64> ** groups_per_worker = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64>*)*(context.worker_count));
        for (int worker_id = 0; worker_id < context.worker_count; worker_id++) groups_per_worker[worker_id] = new std::vector<uint64>;

        std::vector<uint64> *present_groups_found = new std::vector<uint64>;

        if (partial_group_row != NULL){
            // Need to, now, find the rows in the partial file.
            const void *final_partial_group_row_ptr = get_final_ptr(partial_group_row, partial_group_layer);

            while (partial_group_row < final_partial_group_row_ptr){
                partial_group_row = (void*)((uint64)partial_group_layer->record_padding + (uint64)partial_group_row);
                const struct trace_file_partial_row *current_partial_row = (struct trace_file_partial_row *)partial_group_row;
                // Essentially, if the global group number gets found, store the local group number.
                if (std::binary_search(present_groups->begin(), present_groups->end(), current_partial_row->global_group_number)){
                    groups_per_worker[current_partial_row->worker_id]->push_back(current_partial_row->local_group_number);
                    present_groups_found->push_back(current_partial_row->global_group_number);
                }

                partial_group_row = (void*)((uint8*)partial_group_row + sizeof(struct trace_file_partial_row));
            }
        }

        if (partial_group_row == NULL)
            groups_per_worker[context.main_worker_id] = present_groups;

        std::vector<uint64> ** filtered_rows = (std::vector<uint64> **)malloc(sizeof(std::vector<uint64> *)*(main_trace_layer->num_pk_records));

        for (uint32 key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++) filtered_rows[key_idx] = new std::vector<uint64>;

        int iters_made = 0;
        std::vector<uint64>* main_worker_set_difference = nullptr;

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            std::vector<uint64> *local_group_nos = groups_per_worker[worker_id];
            // In case of main worker, we can be in the case where the group was completely within our portion of the table
            // In that case, we'd miss logging it in the local_group_nos.
            if (local_group_nos->size() == 0 && worker_id != context.main_worker_id) continue;

            std::sort(local_group_nos->begin(), local_group_nos->end());

            const struct local_context *bg_context = NULL;
            const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[layer_number - 1];

            void *current_forward_row = NULL;
            if (worker_id == context.main_worker_id){
                current_forward_row = forward_row;
            }else{
                if (map_layer_file(layer_number, worker_id, &current_forward_row, bg_trace_layer->size)){
                    PRINT_ON_DEBUG("Error opening the bg trace file");
                }
            }

            const void *current_final_row = get_final_ptr(current_forward_row, bg_trace_layer);

            while (current_forward_row < current_final_row){
                current_forward_row = (void*)((uint64)bg_trace_layer->record_padding + (uint64)current_forward_row);

                bool found = false;
                found = std::binary_search(local_group_nos->begin(), local_group_nos->end(), ((struct trace_file_forward_row*)current_forward_row)->group_count);
                if (!found && worker_id == context.main_worker_id){
                  // Now, we'd need to compute the set difference. It is deferred till here.
                  if (main_worker_set_difference  == nullptr){
                    std::sort(present_groups_found->begin(), present_groups_found->end());
                    main_worker_set_difference = set_diff(present_groups, present_groups_found);
                  }
                  found = std::binary_search(main_worker_set_difference->begin(), main_worker_set_difference->end(), ((struct trace_file_forward_row*)current_forward_row)->group_count);
                }

                if (found){
                    for (uint32 key_idx = 0; key_idx < bg_trace_layer->num_pk_records; key_idx++){
                        int64 record_key = *GET_PK_FROM_ROW(((struct trace_file_forward_row*)current_forward_row), key_idx);

                        filtered_rows[key_idx]->push_back(record_key);
                    }
                }
                iters_made++;
                current_forward_row = (void*)GET_PK_FROM_ROW(((struct trace_file_forward_row*)current_forward_row), bg_trace_layer->num_pk_records);
            }
        }

        struct infer_result *infer_result_computed = (struct infer_result*)malloc(sizeof(struct infer_result));
        infer_result_computed->ids = filtered_rows;
        infer_result_computed->width = main_trace_layer->num_pk_records;

        return infer_result_computed;
    }

    PG_FUNCTION_INFO_V1(traceprov_infer);

    Datum traceprov_infer(FunctionCallInfo fcinfo){


        ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
        TupleDesc	tupdesc;
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

        return (Datum) 0;
    }

    // Performs inference via the graph.
    PG_FUNCTION_INFO_V1(traceprov_infer_graph);

    Datum traceprov_infer_graph(PG_FUNCTION_ARGS){
        perform_inference_driver();
        PG_RETURN_INT64(0);
    }

    PG_FUNCTION_INFO_V1(traceprov_infer_time);

    // Runs the inference, and returns just the time taken to complete the inference.
    // Note that it is the time to just fill-up the buffer with primary keys.
    // So, it is a good indicator of overhead of inference (rather than materialization)
    Datum traceprov_infer_time(FunctionCallInfo fcinfo){
        
        const int32 layer_number = PG_GETARG_INT32(0);
        const int32 reference_layer = PG_GETARG_INT32(1);
        const int32 subq_layer_number = PG_GETARG_INT32(2);

        auto start = std::chrono::high_resolution_clock::now();

        struct infer_result *infer_result_computed = perform_inference(layer_number, reference_layer, subq_layer_number);

        auto end = std::chrono::high_resolution_clock::now();
        
        UNUSED(infer_result_computed);

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        uint64 duration_time = (uint64)duration.count();

        PG_RETURN_INT64(duration_time);
    }

    PG_FUNCTION_INFO_V1(traceprov_sync_time);

    Datum traceprov_sync_time(FunctionCallInfo fcinfo){
        std::vector<std::string> *messages = new std::vector<std::string>;

        auto start = std::chrono::high_resolution_clock::now();

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context)){
            elog(ERROR, "Error mmaping the shared context for sync!");
        }

	    int final_code = 0;
        const auto worker_local_contexts = traceprov_get_local_contexts(context.worker_count);

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            struct local_context *worker_local_context = worker_local_contexts->at(worker_id);

            for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++){

                if (worker_local_context->cached_layers[layer_id].layer_number){
                    void *ptr = NULL;
                    if(map_layer_file(
                        worker_local_context->cached_layers[layer_id].layer_number,
                        worker_id + 1,
                        &ptr,
                        worker_local_context->cached_layers[layer_id].size
                    )){
                        elog(ERROR, "Error opening layer file!");
                    }
                    char worker_layer[128] = {0};
                    if (ptr == NULL){
                        messages->push_back("Skipping");
                        continue;
                    }
                    sprintf(worker_layer, "(WORKER: %d, Layer: %d)", worker_id + 1, layer_id);
                    messages->push_back(worker_layer);
                    final_code |= (msync(ptr, worker_local_context->cached_layers[layer_id].size*TRACEPROV_PAGE_SIZE, MS_SYNC));
		            if (final_code) {elog(ERROR, "Error doing the msync!");}
   
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        uint64 duration_time = (uint64)duration.count();

        for (std::string s: *messages){
            elog(INFO, "SYNC: %s", s.c_str());
	    }
        elog(INFO, "Final code: %d", final_code);
        PG_RETURN_INT64(duration_time);
    }

    // Prints some useful statistics (like # of pks, # of groups)
    PG_FUNCTION_INFO_V1(traceprov_layer_stat);

    enum TRACEPROV_LAYER_STAT {
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

        NUM_COLUMNS
    };

    Datum traceprov_layer_stat(FunctionCallInfo fcinfo){
        
        ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
        TupleDesc	tupdesc;
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

        if (map_traceprov_shared_context(&context)){
        elog(ERROR, "Error mmaping the shared context");
        }

        Datum record[TRACEPROV_LAYER_STAT::NUM_COLUMNS];
        bool nulls[TRACEPROV_LAYER_STAT::NUM_COLUMNS];
        memset(nulls, 0, sizeof(bool)*TRACEPROV_LAYER_STAT::NUM_COLUMNS);

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            int fd = open(psprintf(TRACEPROV_WORKER_LAYER_MAP, DataDir, worker_id + 1), O_RDONLY);
            if (fd < 0) elog(ERROR, "Error opening the worker laye rmap!");
            void *ptr = mmap(
                NULL,
                sizeof(struct local_context),
                PROT_READ,
                MAP_SHARED,
                fd,
                0
            );
            if (ptr == MAP_FAILED){
                elog(ERROR, "Error mmaping the layer file!");
            }
            struct local_context *worker_local_context = (struct local_context *)ptr;

            for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++){
                
                struct traceprov_aggregate_layer layer = worker_local_context->cached_layers[layer_id];

                if (layer.layer_number == 0) continue;

                const uint32 record_size = layer.record_padding + (layer.num_pk_records)*sizeof(int64);

                const uint64 final_ptr_offset = (uint64)get_final_ptr(NULL, &layer);

                if (final_ptr_offset % record_size){
                elog(INFO, "Expected ptr offset to be multiple of record size at (WORKER: %d, LAYER: %d)!", worker_id, layer_id);
                }
            
                int64 record_count = 0;
                int32 is_sorted_by_group_no = -1;
                record_count = read_all_columns(layer.layer_number , worker_local_context, nullptr)->at(0)->data->size();
                record[TRACEPROV_LAYER_STAT::is_leader_layer] = Int32GetDatum(layer.is_leader_layer);
                record[TRACEPROV_LAYER_STAT::worker_id] = Int32GetDatum(worker_id);
                record[TRACEPROV_LAYER_STAT::layer_id] = Int32GetDatum(layer.layer_number);
                record[TRACEPROV_LAYER_STAT::num_pk_records] = Int32GetDatum(layer.num_pk_records);
                record[TRACEPROV_LAYER_STAT::layer_size] = Int32GetDatum(layer.size);
                record[TRACEPROV_LAYER_STAT::num_groups] = Int32GetDatum(layer.num_groups);
                record[TRACEPROV_LAYER_STAT::layer_number] = Int32GetDatum(layer.layer_number);
                record[TRACEPROV_LAYER_STAT::record_padding] = Int32GetDatum(layer.record_padding);
                record[TRACEPROV_LAYER_STAT::layer_fd] = Int32GetDatum(layer.layer_fd);
                record[TRACEPROV_LAYER_STAT::logged_record_count] = Int64GetDatumFast(record_count);
                record[TRACEPROV_LAYER_STAT::is_sorted_by_group_num] = Int32GetDatum(is_sorted_by_group_no);
                record[TRACEPROV_LAYER_STAT::aggregate_strategy] = Int32GetDatum(layer.aggregate_strategy);
                std::string graphStr = "[";
                for (int i = 0; i < TRACEPROV_BUCKET_COUNT - 1; i++){
                    graphStr = graphStr.append(psprintf("%d", layer.buckets[i]));
                }
                record[TRACEPROV_LAYER_STAT::buckets] = PointerGetDatum(cstring_to_text(graphStr.c_str()));
                record[TRACEPROV_LAYER_STAT::combined_aggregate_layer_number] = Int32GetDatum(layer.combined_aggregate_layer_number);
                record[TRACEPROV_LAYER_STAT::rows_layer_number] = Int32GetDatum(layer.rows_layer_number);
                tuplestore_putvalues(tupstore, tupdesc, record, nulls);
                }
        }
        #if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
        #endif
        return (Datum) 0;
    }

    // Performs inference via the graph.
    PG_FUNCTION_INFO_V1(traceprov_json_graph);

    Datum traceprov_json_graph(PG_FUNCTION_ARGS){
        TraceProvParseContext *context = NULL;
        const List *graphs = deserializeTraceProvDependency(&context, NULL);
        ListCell *graph_cursor;
        StringInfoData buf;
        initStringInfo(&buf);
        std::string graph_str = "{";
        graph_str = graph_str.append("\"graphs\": [");
        bool needsSep = false;
        foreach(graph_cursor, graphs){
            if (needsSep){
                graph_str = graph_str.append(",");
            }
            needsSep = true;
            const TraceProvDependency *graph =  (TraceProvDependency *)lfirst(graph_cursor);
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

    Datum traceprov_parsed_back(PG_FUNCTION_ARGS) {
        TraceProvParseContext *context = NULL;
        char *final_parsed_back = NULL;
        deserializeTraceProvDependency(&context, &final_parsed_back);
        PG_RETURN_TEXT_P(cstring_to_text(final_parsed_back));
    }

    TraceProvRelation *make_traceprov_relation(TraceProvData *data, char *name){
        if (data->size() == 0)
            elog(ERROR, "Expected to have at least 1 column!");
        auto tp_rel = palloc0_object(TraceProvRelation);
        tp_rel->tag = T_TP_RELATION;
        tp_rel->data = data;
        tp_rel->name = name;
        PRINT_ON_VALIDATE("REL: %s, %ld;", name, data->at(0)->data->size());
        return tp_rel;
    }

    static void traceprov_assert_is_in_range(const uint64 column_count, TraceProvColumn *column){
        if (column->second > column_count)
            elog(ERROR, "Table access out of range!");
    }

    TraceProvJoinExpr *make_traceprov_join_expr(
        TraceProvNode *left,
        TraceProvNode *right,
        TraceProvJoinConditions *join_condition,
        std::vector<TraceProvColumn*> *output_columns,
        bool is_left_star = false,
        bool is_right_star = false,
        bool is_single_result = false
    ){
        auto tp_join_exprn = palloc0_object(TraceProvJoinExpr);
        tp_join_exprn->tag = T_TP_JOIN;
        tp_join_exprn->left= left;
        tp_join_exprn->right = right;
        const auto left_column_count = traceprov_get_node_column_count(left);
        const auto right_column_count = traceprov_get_node_column_count(right);
        for(auto condition: *join_condition){
            if (condition->first->first != 1 || condition->second->first != 2)
                elog(ERROR, "Numberig is inconsistent!");
            traceprov_assert_is_in_range(left_column_count, condition->first);
            traceprov_assert_is_in_range(right_column_count, condition->second);
        }
        tp_join_exprn->join_condition = join_condition;
        tp_join_exprn->output_columns = output_columns;
        tp_join_exprn->result = nullptr;
        tp_join_exprn->is_left_star = is_left_star;
        tp_join_exprn->is_right_star = is_right_star;
        tp_join_exprn->is_single_result = is_single_result;
        return tp_join_exprn;
    }

    TraceProvAppend *make_traceprov_append(
        List *nodes
    ){
        auto tp_append = palloc0_object(TraceProvAppend);
        tp_append->tag = T_TP_APPEND;
        if (list_length(nodes) == 0)
            elog(ERROR, "Making append rel with no nodes!");
        ListCell *node_cursor;
        uint64 column_count = 0;
        foreach(node_cursor, nodes){
            if (column_count == 0){
                column_count = traceprov_get_node_column_count((TraceProvNode*)lfirst(node_cursor));
                continue;
            }
            if (column_count != traceprov_get_node_column_count((TraceProvNode*)lfirst(node_cursor)))
                elog(ERROR, "Got differing column count in append!");
        }
        tp_append->nodes = nodes;
        return tp_append;
    }

    TraceProvJoinExpr *make_traceprov_join_from_rel(
        TraceProvNode *left_side, 
        TraceProvNode *right_side, 
        int output_column_side,
        uint64 join_idx_offset
    ){
        
        TraceProvColumn *join_column_1 = new TraceProvColumn(1, join_idx_offset);
        TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
        auto join_conditions = new TraceProvJoinConditions;
        join_conditions->push_back(new std::pair<TraceProvColumn *, TraceProvColumn*>(join_column_1, join_column_2));
        auto output_columns = new std::vector<TraceProvColumn*>(output_column_side - 1);
        for (int i = 0; i < output_column_side - 1; i++){
            output_columns->at(i) = (new TraceProvColumn(2, i + 2));
        }
        
        return make_traceprov_join_expr(
                left_side,
                right_side,
                join_conditions,
                output_columns
            );
    }

    TraceProvJoinExpr *make_traceprov_simple_join(
        TraceProvData *self_logs, 
        TraceProvNode *join_side, 
        TraceProvParseContext *parse_context, 
        const char *rel_name,
        uint64 join_side_idx = 1
    ){

        if (self_logs == nullptr)
            elog(ERROR, "Expected self logs to always be set!");

        auto right_side = (TraceProvNode *)make_traceprov_relation(self_logs, psprintf("%s_%s", rel_name, tp_parse_get_unique_alias(parse_context)));
        
        return make_traceprov_join_from_rel(join_side, right_side, self_logs->size(), join_side_idx);
    }

    bool is_nested_agg(TraceProvDependency *graph){
        ListCell *entry_cursor;
        bool is_pointer = false;
        foreach(entry_cursor, graph->entries){
            const TraceProvEntry *te = (TraceProvEntry *)lfirst(entry_cursor);
            if (te->kind == TP_ENTRY_KIND_POINTER){
                if (is_pointer)
                    elog(ERROR, "Found multiple pointers, not supported, for now.");
                is_pointer = true;
            }
        }
        if (is_pointer && (list_length(graph->entries) != 1) && (list_length(graph->children) != 1))
            elog(ERROR, "In the pointer case, expected the length of graph entries to be 1, for now.");
        return is_pointer;
    }

    static TraceProvNode* expand_child_aggregate(
        TraceProvDependency *parent_graph_head,
        TraceProvNode *match_side,
        struct local_context *local_context,
        TraceProvParseContext *parse_context
    ){
        if (match_side->tag != T_TP_JOIN)
            elog(ERROR, "Expected the match side to always be a join, for now.");

        auto child_graph = (TraceProvDependency *)list_nth(parent_graph_head->children, 0);
        TraceProvJoinExpr *join_side = (TraceProvJoinExpr *)match_side;
        // No longer necessary.
        // if (join_side->output_columns->size() != 1)
        //     elog(ERROR, "Currently, only handling single output columns in nesting..");

        const TraceProvLayerNumber agg_layer_number = child_graph->headNumber;
        auto layer_data = read_all_columns(agg_layer_number, local_context, child_graph);
        TraceProvJoinExpr *join_exprn = make_traceprov_simple_join(
            layer_data,
            (TraceProvNode *)join_side,
            parse_context,
            "intermediate_join",
            traceprov_get_node_column_count(match_side)
        );
        // Prepend to include all, other than the join column.
        for (uint64 col_idx = 1; col_idx < traceprov_get_node_column_count(match_side); col_idx++){
            join_exprn->output_columns->insert(join_exprn->output_columns->begin(), new TraceProvColumn(1, col_idx));
        }
        if (is_nested_agg(child_graph)){
            return expand_child_aggregate(
                child_graph,
                (TraceProvNode *)join_exprn,
                local_context,
                parse_context
            );
        }
        return (TraceProvNode *)join_exprn;
    }

    void get_column_data_at_offset(
        TraceProvColumnData *column_data, 
        std::vector<uint64> *offsets,
        TraceProvColumnData *final_column_data
    ){
        std::vector<uint64> *data = new std::vector<uint64>(offsets->size());
        uint64 idx = 0;
        for (auto offset: *offsets){
            data->at(idx) = (column_data->data->at(offset));
            idx++;
        }
        final_column_data->data = data;
        final_column_data->descriptor = column_data->descriptor;
    }

    // Gets the data at the input offsets.
    TraceProvData *get_data_at_offsets(TraceProvData *data, std::vector<uint64> *offsets, uint64 skip_idx = 0){
        auto new_data = new TraceProvData;
        for (uint64 column_idx = 1; column_idx <= data->size(); column_idx++){
            auto current_column = traceprov_make_empty_column();
            new_data->push_back(current_column);
            if (column_idx ==  skip_idx) continue;
            auto original_data = data->at(column_idx - 1);
            get_column_data_at_offset(original_data, offsets, current_column);
        }
        return new_data;
    }

    static void perform_child_inference(
        const uint8 worker_id,
        TraceProvData *log_data,
        uint64 match_key_idx,
        TraceProvDependency *graph,
        struct local_context *local_context,
        const std::vector<struct local_context *> *local_contexts,
        const uint8 worker_count,
        bool parent_was_parallelized,
        TraceProvParseContext *parse_context,
        List **p_join_exrpns
    ){
        List *derived_joins = NIL;
        const TraceProvLayerNumber layer_number = graph->headNumber;
        struct traceprov_aggregate_layer *layer = &local_context->cached_layers[layer_number - 1];
        const bool was_split = layer->is_leader_layer;
        // In the likely case, it will always be all the original rows.
        std::vector<uint64> *not_combined_offsets = nullptr;
        // std::vector<uint64> *not_combined = match_key;
        // std::vector<TraceProvJoinExpr *> *join_pairs = new std::vector<TraceProvJoinExpr *>;
        std::vector<TraceProvJoinExpr *> *combine_join_pairs = new std::vector<TraceProvJoinExpr *>;
        if (was_split){
            auto match_key = log_data->at(match_key_idx);
            // If it was split, then only consider splitting the input match keys,
            // based on whether they are combined are not.
            std::vector<uint64> *combined_offsets = new std::vector<uint64>();;
            combined_offsets->reserve(match_key->data->size() / 2);
            not_combined_offsets =  new std::vector<uint64>();
            not_combined_offsets->reserve(match_key->data->size() / 2);
            std::vector<uint64> *combined = new std::vector<uint64>();
            for (uint64 offset = 0; offset < match_key->data->size(); offset++){
                const uint64 key = match_key->data->at(offset);
                if (TRACEPROV_GET_IS_COMBINED(key)){
                    combined->push_back(TRACEPROV_STRIP_COMBINED(key));
                    combined_offsets->push_back(offset);
                }else{
                    not_combined_offsets->push_back(offset);
                }
            }
            // In this case, need to consider the combine layer too.
            const TraceProvLayerNumber combine_layer_number = layer->combined_aggregate_layer_number;
            if (combine_layer_number == 0)
                elog(ERROR, "Expected combine layer to always be set!");
         
            auto combined_layer_data = read_all_columns(combine_layer_number, local_context, nullptr);
            TraceProvData **worker_split = palloc0_array(TraceProvData *, worker_count);
            for (int i = 0; i < worker_count; i++){
                auto worker_vector = new std::vector<std::vector<uint64>*>;
                auto col1 = new std::vector<uint64>;
                col1->reserve(combined_layer_data->at(0)->data->size() / worker_count);
                auto col2 = new std::vector<uint64>;
                col2->reserve(combined_layer_data->at(0)->data->size() / worker_count);
                worker_vector->push_back(new std::vector<uint64>);
                worker_vector->push_back(new std::vector<uint64>);
                worker_split[i] = traceprov_make_column_data(worker_vector);
            }
            // Split the combined layer based on each worker id.
            for (long unsigned int i = 0; i < combined_layer_data->at(0)->data->size(); i++){
                // Technically, this can be inferred from the local log entry.
                const uint64 current_current_worker_id = combined_layer_data->at(1)->data->at(i);
                worker_split[current_current_worker_id - 1]->at(0)->data->push_back(combined_layer_data->at(0)->data->at(i));
                worker_split[current_current_worker_id - 1]->at(1)->data->push_back(combined_layer_data->at(2)->data->at(i));
            }
            // Ignore the data at match key idx (since it needs to be replaced by the stripped offsets.)
            TraceProvData *combined_data_at_offsets = get_data_at_offsets(log_data, combined_offsets, match_key_idx+1);
            combined_data_at_offsets->at(match_key_idx)->data = combined;
            if (combined_data_at_offsets->at(match_key_idx)->data != combined)
                elog(ERROR, "Invalid state...");
            // combined_data_at_offsets->assign(match_key_idx, combined);

            TraceProvRelation *combined_relation = make_traceprov_relation(combined_data_at_offsets, psprintf("combined_log_entry_%s",  tp_parse_get_unique_alias(parse_context)));
            TraceProvColumn *join_column_1 = new TraceProvColumn(1, match_key_idx + 1);
            TraceProvColumn *join_column_2 = new TraceProvColumn(2, 1);
            TraceProvColumn *output_column_1 = new TraceProvColumn(2, 2);
            auto join_condition = new TraceProvJoinConditions;
            auto output_column = new std::vector<TraceProvColumn*>;
            join_condition->push_back(new std::pair<TraceProvColumn*, TraceProvColumn*>(join_column_1, join_column_2));
            output_column->push_back(output_column_1);
            for (int i = 0; i < worker_count; i++){
                TraceProvRelation *worker_relation = make_traceprov_relation(worker_split[i], psprintf("partial_join_%s",  tp_parse_get_unique_alias(parse_context)));
                combine_join_pairs->push_back(
                    make_traceprov_join_expr(
                        (TraceProvNode*)combined_relation,
                        (TraceProvNode*)worker_relation,
                        join_condition,
                        output_column,
                        true
                    )
                );
            }
        }
        // Need to over the match key, and need to split into combined and not combined.
        // This is nice, because there can be groups that never get combined (exist completely in the main worker)

        // Need to join worker_split_group_key with combined.
        // Need to discover all the other layers we need to join not_combined with.
        // In the case where the parent was parallelized, it'll need to join not_combined with just our logs.
        // Otherwise, need to, first, check if there are other workers that logged. Just looking at their structs is enough.
        TraceProvData *not_combined_data = nullptr;
        if (not_combined_offsets == nullptr){
            not_combined_data = log_data;
        }else{
            not_combined_data = get_data_at_offsets(log_data, not_combined_offsets);
        }
        auto not_combined_relation = (TraceProvNode *)make_traceprov_relation(
            not_combined_data, 
            psprintf("not_combined_%s", tp_parse_get_unique_alias(parse_context))
        );

        if (parent_was_parallelized){
            auto self_logs = read_all_columns(layer_number, local_context, graph);
            // In this case, need to only look at our logs.
            auto join_node = (make_traceprov_simple_join(
                self_logs,
                not_combined_relation,
                parse_context,
                "base_rel",
                match_key_idx + 1
            ));
            join_node->is_left_star = true;
            TraceProvNode *final_join_node = (TraceProvNode *)join_node;
            if (is_nested_agg(graph)){
                final_join_node = expand_child_aggregate(
                    graph,
                    final_join_node,
                    local_context,
                    parse_context
                );
            }
            derived_joins = lappend(derived_joins, final_join_node);
        }else{
            // Need to discover other worker's logs.
            for (int i = 0; i < worker_count; i++){
                auto worker_logs = read_all_columns(layer_number, local_contexts->at(i), graph);
                if (worker_logs == nullptr) continue;
                auto worker_relation = (TraceProvNode *)make_traceprov_relation(worker_logs, psprintf("base_rel_%s", tp_parse_get_unique_alias(parse_context)));
                auto join_node = make_traceprov_join_from_rel(not_combined_relation, worker_relation, worker_logs->size(), match_key_idx + 1);
                join_node->is_left_star = true;
                TraceProvJoinExpr *combine_join_node = NULL;
                if (combine_join_pairs->size() == 0){
                    if (i != worker_id)
                        elog(ERROR, "Expected to get here only when current id is the worker id");
                }else{
                    combine_join_node = make_traceprov_join_from_rel(
                        (TraceProvNode*)combine_join_pairs->at(i),
                        worker_relation,
                        worker_logs->size(),
                        log_data->size() + 1
                    );
                    for (uint64 init_offset = 0; init_offset < log_data->size(); init_offset++){
                        combine_join_node->output_columns->insert(
                            combine_join_node->output_columns->begin(),
                            new TraceProvColumn(1, init_offset + 1)
                        );
                    }
                }
                auto final_join_node = (TraceProvNode *)join_node;
                auto final_combine_node = (TraceProvNode *)combine_join_node;


                if (is_nested_agg(graph)){
                    final_join_node = expand_child_aggregate(
                        graph,
                        final_join_node,
                        local_contexts->at(i),
                        parse_context
                    );
                    if (final_combine_node){
                        final_combine_node = expand_child_aggregate(
                            graph,
                            final_combine_node,
                            local_contexts->at(i),
                            parse_context
                        );
                    }
                }

                derived_joins = list_concat(
                    derived_joins,
                    final_combine_node == NULL ? list_make1(final_join_node) : list_make2(final_join_node, final_combine_node)
                );
            }
        }
        *p_join_exrpns = list_concat(
            *p_join_exrpns,
            derived_joins
        );
    }

    // Reads all the columns
    // Doesn't go into rows.
    static std::vector<std::vector<uint64> *> *read_all_columns_simple(
        const struct traceprov_aggregate_layer *current_layer,
        const struct local_context *local_context
    ){
        const uint8 worker_id = local_context->worker_id;
        void *log_ptr = NULL;
        if (map_layer_file(current_layer->layer_number, worker_id, &log_ptr, current_layer->size))
            elog(ERROR, "Error mapping (worker: %d, layer: %d)", worker_id, current_layer->layer_number);
        
        const void *final_log_ptr = get_final_ptr(log_ptr, current_layer);
        const uint32 layer_record_padding = current_layer->record_padding; 
        std::vector<std::vector<uint64> *> *current_worker_logs = new std::vector<std::vector<uint64> *>();
        current_worker_logs->reserve(current_layer->num_pk_records);
        const uint64 total_number_of_records = (current_layer->size * TRACEPROV_PAGE_SIZE) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        // Reserve space for next pointers.
        for (uint32 i = 0; i < current_layer->num_pk_records; i++){
            auto data_vec = new std::vector<uint64>;
            data_vec->reserve(total_number_of_records);
            current_worker_logs->push_back(data_vec);
        }


        while (log_ptr < final_log_ptr){
            log_ptr += layer_record_padding;
            uint64 *log_canonical_ptr = (uint64 *)log_ptr;
            for (uint32 entry_id = 0; entry_id < current_layer->num_pk_records; entry_id++, log_canonical_ptr++){
                current_worker_logs->at(entry_id)->push_back(*log_canonical_ptr);
            }
            log_ptr = log_canonical_ptr;
        }
        return current_worker_logs;
    }

    // Reads all columns. Also looks into rows.
    static TraceProvData *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const struct local_context *local_context,
        const TraceProvDependency *dependency
    ){
        const auto evaluate_start = std::chrono::high_resolution_clock::now();
        const struct traceprov_aggregate_layer *current_layer = &local_context->cached_layers[layer_number - 1];
        // In this case, the layer wasn't set.
        if (current_layer->layer_number == 0) return nullptr;
        std::vector<TraceProvOffset *> *data = read_all_columns_simple(current_layer, local_context);

        if (current_layer->rows_layer_number){
            auto row_data = read_all_columns_simple(
                &local_context->cached_layers[current_layer->rows_layer_number - 1],
                local_context
            );
            for (auto second: *row_data){
                data->push_back(second);
            }
        }
        PRINT_ON_VALIDATE("READ LAYER: %d, got: %ld", layer_number, data->at(0)->size());
        TraceProvData *return_data = new TraceProvData;
        for (auto column: *data){
            TraceProvColumnData *column_data = traceprov_make_empty_column();
            column_data->data = column;
            return_data->push_back(column_data);
        }
        dependency = nullptr;

        if (dependency != nullptr){
            if (((uint64)list_length(dependency->entries)) != data->size())
                elog(ERROR, "Expected lengths to be the same!");
            ListCell *entry_cursor = 0;
            foreach(entry_cursor, dependency->entries){
                TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
                TraceProvDescriptor *descriptor = palloc0_object(TraceProvDescriptor);
                descriptor->layer_number = layer_number;
                descriptor->entry = entry;
                return_data->at(foreach_current_index(entry_cursor))->descriptor = descriptor;
            }
        }
        const auto evaluate_end = std::chrono::high_resolution_clock::now();
        PRINT_ON_VALIDATE("read all took: %ld", std::chrono::duration_cast<std::chrono::microseconds>(evaluate_end - evaluate_start).count());
        return return_data;
    }
    
    static void perform_derive_from_log(
        TraceProvDependency *log_dependency,
        const uint8 worker_count,
        TraceProvDerivation *derivation,
        TraceProvParseContext *parsed_back_context,
        const std::vector<struct local_context *> *worker_local_contexts,
        bool allow_both_logs = false
    ){
        const TraceProvLayerNumber log_layer_number = log_dependency->headNumber;
        const uint32 layer_width = list_length(log_dependency->entries);

        std::vector<TraceProvWorkerLayer *> *log_layers = new std::vector<TraceProvWorkerLayer *>();
        for (uint8 worker_id = 0; worker_id < worker_count; worker_id++){
            struct local_context *worker_local_context = worker_local_contexts->at(worker_id);

            struct traceprov_aggregate_layer *candidate_layer = &worker_local_context->cached_layers[log_layer_number - 1];
            if (candidate_layer->layer_number == 0) continue;
            // If it is not 0, it should always be the current log layer...
            if (candidate_layer->layer_number != log_layer_number)
                elog(ERROR, "Got invalid log state!");

            if (candidate_layer->num_pk_records != layer_width)
                elog(ERROR, "Expeced the width to be consistent!");

            log_layers->emplace_back(new TraceProvWorkerLayer(worker_id, candidate_layer));
        }

        if (log_layers->size() == 0)
            elog(ERROR, "Expected the log to always be found!");

        PRINT_ON_VALIDATE("Found %ld log_layers for %d layer number", log_layers->size(), log_layer_number);
    
        if (list_length(log_dependency->entries) > 64)
            elog(ERROR, "Cannot support more than 64 entries for now...");

        ListCell *entry_cursor = NULL;
        List *next_child_pointers = NIL;
        List *normal_log_entries = NIL;
        List *graphs_to_explore = NIL;
        List *graphs_size = NIL;
        bool has_appended_self = false;
        foreach(entry_cursor, log_dependency->entries){
            const TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
            if (entry->kind == TP_ENTRY_KIND_POINTER){
                TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(log_dependency->children, foreach_current_index(entry_cursor));
                if (child_graph == NULL || child_graph->graph_type == TP_INVALID)
                    elog(ERROR, "Got invalid child graph state! Expected to not be null and not to be invalid!");
                next_child_pointers = lappend_int(next_child_pointers, foreach_current_index(entry_cursor));
                graphs_to_explore = lappend(graphs_to_explore, child_graph);
                graphs_size = lappend_int(graphs_size, list_length(log_dependency->entries));
            } else if (entry->kind == TP_ENTRY_KIND_BASE_RELATION && (!has_appended_self)){
                normal_log_entries= lappend_int(normal_log_entries, foreach_current_index(entry_cursor));
                graphs_to_explore = lappend(graphs_to_explore, log_dependency);
                graphs_size = lappend_int(graphs_size, 0);
                has_appended_self = true;
            }
        }
        if ((list_length(next_child_pointers) == 0) && (list_length(normal_log_entries) == 0))
            elog(ERROR, "Found both log entries to be completely empty!");

        if ((list_length(next_child_pointers) > 0) && (list_length(normal_log_entries) > 0) && (!allow_both_logs))
            elog(ERROR, "Found both log entries to be populated... cannot handle this case right now.");

        // If more than 1 final log is found, it implies that final log was parallelized.
        // So, in that case, we don't need to try matching log entries from other workers.
        const bool final_log_was_parallel = log_layers->size() > 1;
        List *current_compute_nodes = NIL;
        for (auto log_pair: *log_layers){
            const struct traceprov_aggregate_layer *current_layer = log_pair->second;
            TraceProvData *current_worker_logs = read_all_columns(
                current_layer->layer_number,
                worker_local_contexts->at(log_pair->first),
                log_dependency
            );

            // Go over all the pointers and split based on each worker.
            ListCell *child_pointer;

            if (list_length(next_child_pointers) > 1)
                elog(ERROR, "Cannot handle more than 1 next child pointer, for now...");

            foreach(child_pointer, next_child_pointers){
                TraceProvDependency *child_graph = ((TraceProvDependency *)list_nth(log_dependency->children, lfirst_int(child_pointer)));
                List *child_join_exprns = NIL;
                perform_child_inference(
                    log_pair->first, 
                    current_worker_logs,
                    lfirst_int(child_pointer),
                    child_graph,
                    worker_local_contexts->at(log_pair->first),
                    worker_local_contexts,
                    worker_count,
                    final_log_was_parallel,
                    parsed_back_context,
                    &child_join_exprns
                );
                auto traceprov_append_node = make_traceprov_append(child_join_exprns);
                current_compute_nodes = lappend(current_compute_nodes, (TraceProvNode *)traceprov_append_node);
            }

            if (list_length(normal_log_entries) && (list_length(next_child_pointers) == 0)){
                TraceProvData *cloned_log = new TraceProvData;
                for(auto column: *current_worker_logs){
                    cloned_log->push_back(column);
                }
                // // Need to also look at any normal log entries (not pointers.)
                // ListCell *normal_log_entry;
                // foreach(normal_log_entry, normal_log_entries){
                //     cloned_log->push_back(current_worker_logs->at(lfirst_int(normal_log_entry)));
                // }
                TraceProvNode *normal_log_relation = (TraceProvNode *)make_traceprov_relation(
                    cloned_log,
                    psprintf("normal_log_%s",  tp_parse_get_unique_alias(parsed_back_context))
                );
                // derivation->derived_join_exprns = lappend(derivation->derived_join_exprns, normal_log_relation);
                current_compute_nodes = lappend(current_compute_nodes, normal_log_relation);
            }
        }

        auto layer_graph_map = new std::unordered_map<TraceProvLayerNumber, TraceProvSublinkMapInferItem *>;
        ListCell *child_graph_cursor;
        foreach(child_graph_cursor, graphs_to_explore){
            ListCell *child_entry_cursor;
            foreach(child_entry_cursor, ((TraceProvDependency *)(lfirst(child_graph_cursor)))->entries){
                TraceProvEntry *entry = (TraceProvEntry *)lfirst(child_entry_cursor);
                if (list_length(entry->sublinks) == 0) continue;

                ListCell *sublink_cursor;
                foreach(sublink_cursor, entry->sublinks){
                    const TraceProvTargetSublinkItem *sublink_item = (TraceProvTargetSublinkItem *)lfirst(sublink_cursor);
                    if (layer_graph_map->find(sublink_item->layer_number) == layer_graph_map->end()){
                        auto new_element = new std::pair<TraceProvJoinConditions *, TraceProvDependency *>(
                            new TraceProvJoinConditions,
                            tp_get_sublink_graph(parsed_back_context, sublink_item->layer_number)
                        );
                        layer_graph_map->insert({sublink_item->layer_number, new_element});
                    }
                    TraceProvSublinkMapInferItem *item = layer_graph_map->at(sublink_item->layer_number);
                    auto join_condition = new std::pair<TraceProvColumn*, TraceProvColumn*>(
                        new TraceProvColumn(1, foreach_current_index(child_entry_cursor) + list_nth_int(graphs_size, foreach_current_index(child_graph_cursor)) + 1),
                        new TraceProvColumn(2, sublink_item->offset_in_key + 1)
                    );
                    item->first->push_back(join_condition);
                }
            }
        }

        auto current_compute_node = make_traceprov_append(current_compute_nodes);
        derivation->derived_join_exprns = lappend(
            derivation->derived_join_exprns, traceprov_make_derived_node(log_dependency->headNumber, (TraceProvNode *)current_compute_node) 
        );
        List *sublink_computation_nodes = NIL;
        for(auto layer_graph_map_item: *layer_graph_map){
            auto child_derivation = new TraceProvDerivation;
            child_derivation->derived_join_exprns = NIL;
            child_derivation->utilized_sublinks = NIL;
            TraceProvSublinkMapInferItem *infer_item = layer_graph_map_item.second;
            TraceProvJoinConditions *join_conditions = infer_item->first;
            // Derive the graph on the child.
            perform_derive_from_log(infer_item->second, worker_count, child_derivation, parsed_back_context, worker_local_contexts, true);
            List *child_join_exprns = NIL;
            ListCell *child_join_cursor;
            foreach(child_join_cursor, child_derivation->derived_join_exprns){
                child_join_exprns = lappend(child_join_exprns, ((TraceProvDerivedNode *)lfirst(child_join_cursor))->node);
            }
            TraceProvNode *traceprov_child_append_node = (TraceProvNode *)make_traceprov_append(child_join_exprns);
            if (join_conditions->size()){
                traceprov_child_append_node = (TraceProvNode *)make_traceprov_join_expr(
                    (TraceProvNode*)current_compute_node,
                    traceprov_child_append_node,
                    join_conditions,
                    nullptr,
                    false,
                    // So that we don't have to explictly know how it does it.
                    true,
                    // temp.
                    true
                );
            }
            sublink_computation_nodes = lappend(
                sublink_computation_nodes, 
                traceprov_make_derived_node(
                    infer_item->second->headNumber,
                    traceprov_child_append_node
                )
            );
            derivation->utilized_sublinks = lappend_int(derivation->utilized_sublinks, infer_item->second->headNumber);
            derivation->utilized_sublinks = list_concat(derivation->utilized_sublinks, child_derivation->utilized_sublinks);
        }

        if (list_length(sublink_computation_nodes)){
            derivation->derived_join_exprns = list_concat(derivation->derived_join_exprns, sublink_computation_nodes);
        }
    }

    char *get_sample_values(TraceProvData *data){
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfo(&buf, "[SAMPLE OF %ld]: sample_values: {", data->at(0)->data->size());
        for (uint64 i = 0; i < Min(data->at(0)->data->size(), 10); i++){
            appendStringInfo(&buf, "[");
            for (uint64 col = 0; col < data->size(); col++){
                if (col > 0) appendStringInfo(&buf, ",");
                appendStringInfo(&buf, "%ld", data->at(col)->data->at(i));
            }
            appendStringInfo(&buf, "]");
        }
        appendStringInfo(&buf, "}");
        return buf.data;
    }

    char *traceprov_node_to_string(const TraceProvNode *node){
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '(');
        if (node->tag == T_TP_RELATION){
            const TraceProvRelation *relation = (TraceProvRelation *)node;
            appendStringInfo(&buf, "RELATION: %s (columns: %ld, count: %ld)", relation->name, relation->data->size(), relation->data->at(0)->data->size());
            // appendStringInfo(&buf, "sample_values: %s", get_sample_values(relation->data));
        }else if (node->tag == T_TP_JOIN){
            const TraceProvJoinExpr *join_expr = (TraceProvJoinExpr *)node;
            appendStringInfo(&buf, "%s", traceprov_node_to_string(join_expr->left));
            appendStringInfoString(&buf, " JOIN ");
            appendStringInfo(&buf, "%s", traceprov_node_to_string(join_expr->right));
            appendStringInfoString(&buf, " - OUT: [");
            if (join_expr->output_columns != nullptr){
                for (long unsigned int i = 0; i < join_expr->output_columns->size(); i++){
                    auto out_column = join_expr->output_columns->at(i);
                    appendStringInfo(&buf, "(%ld, %ld)", out_column->first, out_column->second);
                }
            }

            appendStringInfoString(&buf, "]");
            appendStringInfoString(&buf, " - ON: [");
            for (long unsigned int  i = 0; i < join_expr->join_condition->size(); i++){
                auto out_column = join_expr->join_condition->at(i);
                appendStringInfo(
                    &buf, 
                    "(%ld, %ld)=(%ld, %ld)", 
                    out_column->first->first, 
                    out_column->first->second, 
                    out_column->second->first, 
                    out_column->second->second
                );
            }
            appendStringInfoString(&buf, "]");
            appendStringInfo(&buf, "(is_left_star: %d, is_right_star: %d)", join_expr->is_left_star, join_expr->is_right_star);

            if (join_expr->result != nullptr){
                appendStringInfo(&buf, " - RESULT_SIZE: [%ld]", join_expr->result->at(0)->data->size());
                // appendStringInfo(&buf, "sample_values: %s", get_sample_values(join_expr->result));
            }

        } else if (node->tag == T_TP_APPEND){
            const TraceProvAppend *tp_append = (TraceProvAppend *)node;
            appendStringInfoString(&buf, "APPEND [");
            ListCell *node_cursor;
            foreach(node_cursor, tp_append->nodes){
                TraceProvNode *child_node = (TraceProvNode *)lfirst(node_cursor);
                appendStringInfoString(&buf, traceprov_node_to_string(child_node));
            }
            appendStringInfoString(&buf, "]");
        }
        appendStringInfoChar(&buf, ')');
        return buf.data;
    }

    typedef TraceProvData TraceProvJoinResult;
    typedef std::pair<uint64, uint64> TraceProvTuple;
    // Evaluates the join exprn
    static TraceProvJoinResult* traceprov_evaluate_join_exprn_key_count_1(
        TraceProvColumnData *left_column,
        TraceProvColumnData *right_column,
        bool is_single_result
     ){
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
        for (uint64 left_offset = 0; left_offset < left_column->data->size(); left_offset++){
            left_column_clone->push_back(TraceProvTuple(left_column->data->at(left_offset), left_offset));
        }

        std::sort(left_column_clone->begin(), left_column_clone->end());
        for (uint64 offset = 0; offset < right_column->data->size(); offset++){
            const auto match_key = right_column->data->at(offset);
            auto it = (std::lower_bound(
                left_column_clone->begin(), 
                left_column_clone->end(), 
                match_key,
                [](const TraceProvTuple& p, const uint64 val) {
                    return p.first < val;
                }
            ));

            while (it != left_column_clone->end()){
                bool has_seen_before = false;
                if (it->first == match_key){
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

    static std::vector<std::vector<uint64>*> *traceprov_flatten_data(TraceProvData *input_data){
        auto row_count = input_data->at(0)->data->size();
        auto flattened = new  std::vector<std::vector<uint64>*>;
        flattened->reserve(row_count);
        for (uint64 row_idx = 0; row_idx < row_count; row_idx++){
            auto row_data = new std::vector<uint64>;
            row_data->reserve(input_data->size());
            for (uint64 col_idx = 0; col_idx < input_data->size(); col_idx++){
                row_data->push_back(input_data->at(col_idx)->data->at(row_idx));
            }
            flattened->push_back(row_data);
        }
        return flattened;
    }

    bool vector_compare_is_less(const std::vector<uint64> *left, const std::vector<uint64> *right){
        if (left->size() != right->size())
            elog(ERROR, "Expected both sides to be of same size!");
        
        for (uint64 col_idx = 0; col_idx < left->size(); col_idx++){
            uint64 left_value = left->at(col_idx);
            uint64 right_value = right->at(col_idx);
            if (left_value == right_value)
                continue;

            return left_value < right_value;
        }
        // In the case where they are all equal, return false...
        return false;
    }

    bool vector_compare_is_equal(const std::vector<uint64> *left, const std::vector<uint64> *right){
        if (left->size() != right->size())
            elog(ERROR, "Expected both sides to be of same size!");

        for (uint64 col_idx = 0; col_idx < left->size(); col_idx++){
            uint64 left_value = left->at(col_idx);
            uint64 right_value = right->at(col_idx);
            if (left_value != right_value)
                return false;
        }
        return true;
    }

    typedef std::pair<std::vector<uint64>*, uint64> TraceProvMultipleTuple;

    static TraceProvJoinResult *traceprov_evaluate_join_exprn_many_key(
        TraceProvData *left_columns,
        TraceProvData *right_columns,
        bool is_single_result
    ){
        auto offsets_found = new TraceProvJoinResult;
        auto left_offsets = traceprov_make_empty_column();
        auto right_offsets = traceprov_make_empty_column();
        offsets_found->push_back(left_offsets);
        offsets_found->push_back(right_offsets);

        if (left_columns->at(0)->data->size() == 0 || right_columns->at(0)->data->size() == 0)
            return offsets_found;

        left_offsets->data->reserve(left_columns->at(0)->data->size() / 2);
        right_offsets->data->reserve(right_columns->at(0)->data->size() / 2);
        auto left_flattened = traceprov_flatten_data(left_columns);
        auto right_flattened = traceprov_flatten_data(right_columns);
        auto left_flattened_cloned = new std::vector<TraceProvMultipleTuple>();
        left_flattened_cloned->reserve(left_flattened->size());
        for (uint64 left_offset = 0; left_offset < left_flattened->size(); left_offset++){
            left_flattened_cloned->push_back(TraceProvMultipleTuple(left_flattened->at(left_offset), left_offset));
        }
        auto vector_tuple_is_less = [](const TraceProvMultipleTuple& left, const TraceProvMultipleTuple& right) {
            return vector_compare_is_less(left.first, right.first);
        };
        std::sort(
            left_flattened_cloned->begin(), 
            left_flattened_cloned->end(), 
            vector_tuple_is_less
        );

        auto vector_value_is_less = [](const TraceProvMultipleTuple& left, const TraceProvOffset *right) {
            return vector_compare_is_less(left.first, right);
        };

        for (uint64 offset = 0; offset < right_flattened->size(); offset++){
            const auto match_key = right_flattened->at(offset);
            auto it = (std::lower_bound(
                left_flattened_cloned->begin(), 
                left_flattened_cloned->end(), 
                match_key, 
                vector_value_is_less
            ));
            while (it != left_flattened_cloned->end()){
                bool has_seen_before = false;
                if (vector_compare_is_equal(it->first, match_key)){
                    if (has_seen_before && is_single_result)
                        elog(ERROR, "Got multiple matches, when only one was expected!");
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

    static TraceProvData* traceprov_evaluate_join_exprn(
        TraceProvJoinExpr *join_exprn, 
        TraceProvEvaluateNodeContext *eval_context
    ){
        // If this join exprn is already evaluated, simply return the last result.
        if (join_exprn->result != nullptr)
            return join_exprn->result;
        
        TraceProvData *left_result = traceprov_evaluate_node(join_exprn->left, eval_context);
        TraceProvData *right_result = traceprov_evaluate_node(join_exprn->right, eval_context);
        if (join_exprn->is_single_result){
            if (eval_context->should_dump){
                traceprov_dump_data_to_csv(left_result, psprintf(DEFINE_TRACE_PROV_FILE("/left_data_single_result.csv"), DataDir));
                traceprov_dump_data_to_csv(right_result, psprintf(DEFINE_TRACE_PROV_FILE("/right_data_single_result.csv"), DataDir));
                PRINT_ON_VALIDATE("Logging join exprn: %s!", traceprov_node_to_string((TraceProvNode *)join_exprn));
            }
        }

        // This is poor mans index nested loop join.
        // The left result is guaranteed to be smaller than the right side, so it is sorted.

        // Collect all the columns that need to be sorted.
        // This is the common case.
        // Othercases can happen in sublinks.
        TraceProvJoinResult *offsets = nullptr;
        if (join_exprn->join_condition->size() == 1){
            auto join_pair = join_exprn->join_condition->at(0);
            if (VALIDATE_MODE){
                if (join_pair->first->first != 1 || join_pair->second->first != 2)
                    elog(ERROR, "Got unexpected numbering..");
            }

            TraceProvColumnData *left_columns =  left_result->at(join_pair->first->second - 1);
            TraceProvColumnData *right_columns =  right_result->at(join_pair->second->second - 1);
            offsets = traceprov_evaluate_join_exprn_key_count_1(
                left_columns,
                right_columns,
                join_exprn->is_single_result
            );
        }else{
            TraceProvData *left_columns = new TraceProvData;
            TraceProvData *right_columns = new TraceProvData;
            for (auto join_pair: *join_exprn->join_condition){
                if (VALIDATE_MODE){
                    if (join_pair->first->first != 1 || join_pair->second->first != 2)
                        elog(ERROR, "Got unexpected numbering...");
                }

                left_columns->push_back(left_result->at(join_pair->first->second - 1));
                right_columns->push_back(right_result->at(join_pair->second->second - 1));
            }
            offsets = traceprov_evaluate_join_exprn_many_key(
                left_columns,
                right_columns,
                join_exprn->is_single_result
            );
        }
        if (VALIDATE_MODE){
            if (offsets->at(0)->data->size() == 0){
                if (eval_context->should_dump){
                    traceprov_dump_data_to_csv(left_result, psprintf(DEFINE_TRACE_PROV_FILE("/left_data_no_result.csv"), DataDir));
                    traceprov_dump_data_to_csv(right_result, psprintf(DEFINE_TRACE_PROV_FILE("/right_data_no_result.csv"), DataDir));
                    elog(INFO, "Got 0 as the join result: %s!", traceprov_node_to_string((TraceProvNode *)join_exprn));
                }else{
                    // elog(INFO, "Got 0 as the join result!");
                }
            }
        }

        TraceProvData *result = new TraceProvData;
        if(join_exprn->is_left_star){
            auto left_offsets = offsets->at(0);
            for (uint64 i = 0; i < left_result->size(); i++){
                auto filtered_column_result = traceprov_make_empty_column();
                get_column_data_at_offset(
                    left_result->at(i),
                    left_offsets->data,
                    filtered_column_result
                );
                result->push_back(filtered_column_result);
            }
        }
        
        if(join_exprn->is_right_star){
            auto right_offsets = offsets->at(1);
            for (uint64 i = 0; i < right_result->size(); i++){
                auto filtered_column_result = traceprov_make_empty_column();
                get_column_data_at_offset(
                    right_result->at(i),
                    right_offsets->data,
                    filtered_column_result
                );
                result->push_back(filtered_column_result);
            }
        }

        if (join_exprn->output_columns != nullptr){
            for (auto output_column: *join_exprn->output_columns){
                if (
                    (output_column->first == 1 && join_exprn->is_left_star) ||
                    (output_column->first == 2 && join_exprn->is_right_star)
                ){
                    elog(INFO, "Skipping because of left/right *");
                    continue;
                }
                auto filtered_column_result = traceprov_make_empty_column();
                auto result_offsets = offsets->at(output_column->first - 1);
                auto data_column = output_column->first == 1 ? left_result : right_result;
                get_column_data_at_offset(
                    data_column->at(output_column->second - 1),
                    result_offsets->data,
                    filtered_column_result
                );
                result->push_back(filtered_column_result);
            }
        }

        uint64 final_result_size = 0;
        for (uint64 i = 0; i < result->size(); i++){
            if (i == 0){
                final_result_size = result->at(i)->data->size();
            }else{
                if (VALIDATE_MODE){
                    if (final_result_size != result->at(i)->data->size())
                        elog(ERROR, "Expected final join result to be of same columns: %ld, %ld", final_result_size, result->at(i)->data->size());
                }
            }
        }
        join_exprn->result = result;
        return result;
    }

    uint64 traceprov_get_node_column_count(TraceProvNode *node){
        if (node->tag == T_TP_RELATION){
            TraceProvRelation *relation = (TraceProvRelation *)node;
            return relation->data->size();
        } else if (node->tag == T_TP_JOIN){
            TraceProvJoinExpr *join_exprn = (TraceProvJoinExpr *)node;
            uint64 left_count = 0, right_count = 0, mid_count = 0;
            if (join_exprn->is_left_star){
                left_count = traceprov_get_node_column_count(join_exprn->left);
            }
            if (join_exprn->is_right_star){
                right_count = traceprov_get_node_column_count(join_exprn->right);
            }
            if (join_exprn->output_columns != nullptr){
                for (auto output_column: *join_exprn->output_columns){
                    if (
                        (output_column->first == 1 && join_exprn->is_left_star) ||
                        (output_column->first == 2 && join_exprn->is_right_star)
                    ){
                        elog(INFO, "Skipping because of left/right *");
                        continue;
                    }
                    if (output_column->first == 1) left_count = 0;
                    if (output_column->first == 2) right_count = 0;
                    mid_count++;
                }
            }

            return left_count + mid_count + right_count;
        } else if (node->tag == T_TP_APPEND){
            TraceProvAppend *append = (TraceProvAppend *)node;
            return traceprov_get_node_column_count((TraceProvNode *)list_nth(append->nodes, 0));
        }
        elog(ERROR, "Got unexepected node: %d", node->tag);
        return 0;
    }


    static TraceProvData *traceprov_evaluate_append(TraceProvAppend *append_node, TraceProvEvaluateNodeContext *eval_context){
        ListCell *node_cursor;
        TraceProvNode *first_node = (TraceProvNode *)list_nth(append_node->nodes, 0);
        const TraceProvData *result = traceprov_evaluate_node(first_node, eval_context);
        const uint64 initial_result_width = result->size();
        TraceProvData *final_result = new TraceProvData;
        for (uint64 col_idx = 0; col_idx < result->size(); col_idx++){
            TraceProvColumnData *column_data = traceprov_make_empty_column();
            // This is very very over-approximate.
            column_data->data->reserve(result->at(col_idx)->data->size()*list_length(append_node->nodes));
            for (auto value: *result->at(col_idx)->data){
                column_data->data->push_back(value);
            }
            column_data->descriptor = result->at(col_idx)->descriptor;
            final_result->push_back(column_data);
        }
        for_each_from(node_cursor, append_node->nodes, 1){
            TraceProvData *current = traceprov_evaluate_node((TraceProvNode *)lfirst(node_cursor), eval_context);
            if (current->size() != initial_result_width)
                elog(ERROR, "Expected all the nodes in an append block to have the same width");
            for (uint64 col_cursor = 0; col_cursor < initial_result_width; col_cursor++){
                for (uint64 row_cursor = 0; row_cursor < current->at(col_cursor)->data->size(); row_cursor++){
                    final_result->at(col_cursor)->data->push_back(current->at(col_cursor)->data->at(row_cursor));
                }
            }
        }
        return final_result;
    }

    static TraceProvData *traceprov_evaluate_node(TraceProvNode *node, TraceProvEvaluateNodeContext *eval_context){
        if (node->tag == T_TP_RELATION){
            TraceProvRelation *relation = (TraceProvRelation *)node;
            if (eval_context->should_dump){
                traceprov_dump_data_to_csv(relation->data, psprintf(DEFINE_TRACE_PROV_FILE("/%s.csv"), DataDir, relation->name), true);
            }
            return relation->data;
        } else if (node->tag == T_TP_JOIN){
            const auto evaluate_start = std::chrono::high_resolution_clock::now();
            TraceProvJoinExpr *join_exprn = (TraceProvJoinExpr *)node;
            auto result = traceprov_evaluate_join_exprn(join_exprn, eval_context);
            const auto evaluate_end = std::chrono::high_resolution_clock::now();
            PRINT_ON_VALIDATE("Join took: %ld", std::chrono::duration_cast<std::chrono::microseconds>(evaluate_end - evaluate_start).count());
            return result;
        } else if (node->tag == T_TP_APPEND){
            const auto evaluate_start = std::chrono::high_resolution_clock::now();
            TraceProvAppend *append_node = (TraceProvAppend *)node;
            auto result = traceprov_evaluate_append(append_node, eval_context);
            const auto evaluate_end = std::chrono::high_resolution_clock::now();
            PRINT_ON_VALIDATE("Append took: %ld", std::chrono::duration_cast<std::chrono::microseconds>(evaluate_end - evaluate_start).count());
            return result;
        }
        elog(ERROR, "Invalid tag: %d", node->tag);
    }

    static void traceprov_dump_data_to_csv(TraceProvData *data, char *file_name, bool include_headers){
        std::string csv_str = "";
        if (include_headers){
            for (uint64 col_idx = 0; col_idx < data->size(); col_idx++){
                if (col_idx > 0) csv_str += ",";
                csv_str += psprintf("column_%ld", col_idx);
            }
            csv_str += "\n";
        }
        for (uint64 row_idx = 0; row_idx < data->at(0)->data->size(); row_idx++){
            if (row_idx > 0) csv_str += "\n";
            std::string row_str = "";
            for (uint64 column_idx = 0; column_idx < data->size(); column_idx++){
                if (column_idx > 0) row_str += ",";
                row_str += std::to_string(data->at(column_idx)->data->at(row_idx));
            }
            csv_str += row_str;
        }
        std::ofstream out(file_name);
        out << csv_str;
        out.close();
    }

    static char *traceprov_get_column_name_idx(char *alias, uint64 column_idx){
        if (alias == nullptr)
            return psprintf("column_%ld", column_idx);
        return psprintf("%s.column_%ld", alias, column_idx);
    }

    static char *traceprov_get_column_select(char *alias, uint64 column_count, bool add_bigint_cast=false){
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        for (uint64 column_idx = 0; column_idx < column_count; column_idx++){
            if (column_idx > 0) appendStringInfoChar(&sql_repr, ',');
            appendStringInfoString(&sql_repr, traceprov_get_column_name_idx(alias, column_idx));
            if (add_bigint_cast){
               appendStringInfoString(&sql_repr, "::bigint");
            }
        }
        return sql_repr.data;
    }

    static char *traceprov_relation_to_sql(TraceProvRelation *relation){
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfoString(&sql_repr, "SELECT ");
        appendStringInfoString(&sql_repr, traceprov_get_column_select(relation->name, relation->data->size(), true));
        appendStringInfo(&sql_repr, " FROM %s", relation->name);
        return sql_repr.data;
    }

    static char *expand_alias(const char *alias_name, const uint64 column_count){
        // Expands alias such that it is as table(col0, col1, col2...)
        StringInfoData alias_repr;
        initStringInfo(&alias_repr);
        auto select = traceprov_get_column_select(nullptr, column_count);
        appendStringInfo(&alias_repr, "%s(%s)", alias_name, select);
        return alias_repr.data;
    }

    static char *traceprov_join_to_sql(TraceProvJoinExpr *join_expr, TraceProvParseContext *context){
        char *left_node_raw_sql = traceprov_node_to_sql(join_expr->left, context);
        char *right_node_raw_sql = traceprov_node_to_sql(join_expr->right, context);
        char *left_alias = join_expr->left->alias_name;
        char *right_alias = join_expr->right->alias_name;
        char *left_alias_expanded =expand_alias(left_alias, traceprov_get_node_column_count(join_expr->left));
        char *right_alias_expanded =expand_alias(right_alias, traceprov_get_node_column_count(join_expr->right));

        char *left_node_sql = psprintf("(%s) as %s", left_node_raw_sql, left_alias_expanded);
        char *right_node_sql = psprintf("(%s) as %s", right_node_raw_sql, right_alias_expanded);
        StringInfoData sql_repr;
        initStringInfo(&sql_repr);
        appendStringInfoString(&sql_repr, "SELECT ");
        bool did_append = false;

        // Expand all lefts.
        if (join_expr->is_left_star){
            did_append = true;
            const uint64 left_column_count = traceprov_get_node_column_count(join_expr->left);
            appendStringInfoString(&sql_repr, traceprov_get_column_select(left_alias, left_column_count));
        }

        // Expand all the rights.
        if (join_expr->is_right_star){
            if (did_append) appendStringInfoString(&sql_repr, ", ");
            did_append = true;
            const uint64 right_column_count = traceprov_get_node_column_count(join_expr->right);
            appendStringInfoString(&sql_repr, traceprov_get_column_select(right_alias, right_column_count));
        }

        if (join_expr->output_columns != nullptr){
            for (auto output_column: *join_expr->output_columns){
                if (
                    (output_column->first == 1 && join_expr->is_left_star) ||
                    (output_column->first == 2 && join_expr->is_right_star)
                ){
                    elog(INFO, "Skipping because of left/right *");
                    continue;
                }
                if (did_append) appendStringInfoString(&sql_repr, ", ");
                auto alias_name = output_column->first == 1 ? left_alias : right_alias;
                appendStringInfoString(&sql_repr, traceprov_get_column_name_idx(alias_name, output_column->second - 1));
                did_append = true;
            }
        }
        // Add the from claause.
        StringInfoData join_repr;
        initStringInfo(&join_repr);
        bool did_add_in_join = false;
        for (auto join_condition: *join_expr->join_condition){
            if (did_add_in_join)
                appendStringInfoString(&join_repr, " AND ");
            did_add_in_join = true;
            auto left_column = join_condition->first;
            auto right_column = join_condition->second;
            if (VALIDATE_MODE){
                if (left_column->first != 1 || right_column->first != 2)
                    elog(ERROR, "Got invalid numbering!");
            }
            appendStringInfo(
                &join_repr, 
                "%s=%s", 
                traceprov_get_column_name_idx(left_alias, left_column->second - 1),
                traceprov_get_column_name_idx(right_alias, right_column->second - 1)
            );
        }
        appendStringInfo(&sql_repr, " FROM %s JOIN %s ON (%s) ", left_node_sql, right_node_sql, join_repr.data);
        return sql_repr.data;
    }

    static char *traceprov_append_to_sql(TraceProvAppend *append, TraceProvParseContext *context){
        if (list_length(append->nodes) == 1){
            TraceProvNode *single_node =  (TraceProvNode*)list_nth(append->nodes, 0);
            char *node_sql = traceprov_node_to_sql(single_node, context);
            // Don't bother generating a new alias for this specific case.
            // Just use whatever the child is.
            append->alias_name = single_node->alias_name;
            return node_sql;
        }
        append->alias_name = tp_parse_get_unique_alias(context);
        StringInfoData append_repr;
        initStringInfo(&append_repr);
        ListCell *node_cursor;
        foreach(node_cursor, append->nodes){
            if (foreach_current_index(node_cursor) > 0)
                appendStringInfoString(&append_repr, " UNION ALL ");
            TraceProvNode *node = (TraceProvNode *)lfirst(node_cursor);
            char *node_sql = traceprov_node_to_sql(node, context);
            appendStringInfo(&append_repr, " (%s) ", node_sql);
        }
        return append_repr.data;
    }

    static char *traceprov_node_to_sql(TraceProvNode *node, TraceProvParseContext *context){
        if (node->tag == T_TP_RELATION){
            node->alias_name = tp_parse_get_unique_alias(context);
            return traceprov_relation_to_sql((TraceProvRelation *)node);
        }
        if (node->tag == T_TP_JOIN){
            node->alias_name = tp_parse_get_unique_alias(context);
            return traceprov_join_to_sql((TraceProvJoinExpr *)node, context);
        }
        if (node->tag == T_TP_APPEND){
            return traceprov_append_to_sql((TraceProvAppend *)node, context);
        }
        elog(ERROR, "Found handling invalid node: %d", node->tag);
    }

    static List *get_used_sublinks_in_dependency(const TraceProvDependency *dependency){
        List *used_layer_numbers = NIL;
        ListCell *cursor;
        foreach(cursor, dependency->entries){
            TraceProvEntry *entry = (TraceProvEntry *)lfirst(cursor);
            if (entry->kind == TP_ENTRY_KIND_POINTER){
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(dependency->children, foreach_current_index(cursor));
                used_layer_numbers = list_concat(used_layer_numbers, get_used_sublinks_in_dependency(child_graph));
            }else if (entry->kind == TP_ENTRY_KIND_BASE_RELATION) {
                ListCell *sublink_cursor;
                foreach(sublink_cursor, entry->sublinks){
                    TraceProvTargetSublinkItem *item = (TraceProvTargetSublinkItem *)lfirst(sublink_cursor);
                    used_layer_numbers = lappend_int(used_layer_numbers, item->layer_number);
                }
            }else{
                elog(ERROR, "Only handling pointer or base relation for now..");
            }
        }
        return used_layer_numbers;
    }

    static List* get_used_sublinks(List *graphs){
        // Sublinks can refer to other sublinks.
        // So, discover those cases too.
        List *used_sublinks = NIL;
        ListCell *graph_cursor;
        foreach(graph_cursor, graphs){
            used_sublinks = list_concat(
                used_sublinks,
                get_used_sublinks_in_dependency(
                    (TraceProvDependency *)lfirst(graph_cursor)
                )
            );
        }
        return used_sublinks;
    }

    static std::unordered_map<TraceProvLayerNumber, TraceProvData *>* perform_derivation(
        char **derivation_spec,
        TraceProvEvaluateNodeContext *eval_context
    ){
        #define PERFORM_IF_USED(X) if (derivation_spec != NULL) X
        TraceProvParseContext *parsed_back_context = NULL;
        List *graphs = deserializeTraceProvDependency(&parsed_back_context, NULL);
        struct traceprov_shared_context shared_context;
        List *used_sublinks = get_used_sublinks(list_concat_copy(graphs, GET_ROOT_CONTEXT(parsed_back_context)->properties->sublinkMap));

        // NOTE: Shared context is used just to get the total number of workers.
        if (map_traceprov_shared_context(&shared_context))
            elog(ERROR, "Error mapping the shared context");
        
        const uint8 worker_count = shared_context.worker_count;
        ListCell *graph_cursor;
        auto derivation = new TraceProvDerivation;
        derivation->derived_join_exprns = NIL;
        derivation->utilized_sublinks = NIL;
        const auto worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
        foreach(graph_cursor, graphs){
            TraceProvDependency *graph = (TraceProvDependency *)lfirst(graph_cursor);
            // The top level graph should always be the simple log.
            if (graph->graph_type != TraceProvGraphKind::TP_LOG)
                elog(ERROR, "Expected the top level graph to always be a TP_LOG. Got %d", graph->graph_type);
            perform_derive_from_log(graph, worker_count, derivation, parsed_back_context, worker_local_contexts);
        }
        ListCell *sublink_cursor;
        foreach(sublink_cursor, parsed_back_context->properties->sublinkMap){
            TraceProvDependency *child_sublink = (TraceProvDependency*)lfirst(sublink_cursor);
            if (traceprov_find_int_list(used_sublinks, child_sublink->headNumber)) continue;
            // Sublinks without any correlation can also exist.
            // This catches those cases (but we need to be careful and not add any that has correlation)
            perform_derive_from_log(child_sublink, worker_count, derivation, parsed_back_context, worker_local_contexts);
        }

        ListCell *derivation_cursor;
        uint64 final_result_size = 0;
        StringInfoData buf;
        PERFORM_IF_USED(initStringInfo(&buf));
        PERFORM_IF_USED(appendStringInfoChar(&buf, '['));

        auto derived_node_map = new std::unordered_map<TraceProvLayerNumber, TraceProvData *>;
        foreach(derivation_cursor, derivation->derived_join_exprns){
            uint64 file_idx = foreach_current_index(derivation_cursor) + 1;

            PERFORM_IF_USED(if (file_idx > 1) appendStringInfoChar(&buf, ','));

            TraceProvDerivedNode *derived_node = (TraceProvDerivedNode *)lfirst(derivation_cursor); 
            // We should never see a derived node again. Assert just that.
            if (derived_node_map->find(derived_node->layer_number) != derived_node_map->end()){
                elog(ERROR, "Layer number %d is being evaluated again!", derived_node->layer_number);
            }
            PRINT_ON_VALIDATE("TRACEPROV_EXPRN_PRE_EVALUATE: %s", traceprov_node_to_string(derived_node->node));
            char *node_sql = NULL;
            PERFORM_IF_USED((node_sql = traceprov_node_to_sql(derived_node->node, parsed_back_context)));
            PRINT_ON_VALIDATE("TRACEPROV_EXPRN_PRE_EVALUATE_SQL: %s", node_sql);
            
            const auto evaluate_start = std::chrono::high_resolution_clock::now();
            TraceProvData *node_result = traceprov_evaluate_node(derived_node->node, eval_context);
            const auto evaluate_end = std::chrono::high_resolution_clock::now();
            const auto duration = evaluate_end - evaluate_start;
            const uint64 duration_time = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            auto dump_file_name = psprintf(DEFINE_TRACE_PROV_FILE("/final_output_%d_dump.csv"), DataDir,  derived_node->layer_number);

            if (eval_context->should_dump){
                // eh, dumping the header here is useful for handling things easily downstream.
                traceprov_dump_data_to_csv(node_result, dump_file_name, true);                
            }

            PRINT_ON_VALIDATE("TRACEPROV_EXPRN (COUNT: %ld): %s", node_result->at(0)->data->size(), traceprov_node_to_string(derived_node->node));
            PRINT_ON_VALIDATE("TRACEPROV_EXPRN (COUNT: %ld)", node_result->at(0)->data->size());
            final_result_size += node_result->at(0)->data->size();
            PRINT_ON_VALIDATE("Sample result: %s", get_sample_values(node_result));

            PERFORM_IF_USED(appendStringInfo(
                &buf, 
                "{\"idx\": %d, \"size\": %ld, \"width\": %ld, \"sql\": \"%s\", \"evaluate_time\": \"%ld\"}",
                derived_node->layer_number,
                node_result->at(0)->data->size(),
                node_result->size(),
                node_sql,
                duration_time
            ));
            derived_node_map->insert({derived_node->layer_number, node_result});
        }
        PERFORM_IF_USED(appendStringInfoChar(&buf, ']'));
        PRINT_ON_VALIDATE("Final result size: %ld", final_result_size);
        PERFORM_IF_USED(PRINT_ON_VALIDATE("Returned spec: %s", buf.data);)
        PERFORM_IF_USED((*derivation_spec = buf.data));
        return derived_node_map;
    }

    // static derive_from_log(const uint8 worker_count)
    // The main entry point to all the derivation.
    PG_FUNCTION_INFO_V1(traceprov_perform_derivation);

    Datum traceprov_perform_derivation(PG_FUNCTION_ARGS){
        const uint64 result_to_return = PG_GETARG_INT64(0);
        const bool should_dump = PG_GETARG_BOOL(1);
        TraceProvEvaluateNodeContext *eval_context = palloc0_object(TraceProvEvaluateNodeContext);
        eval_context->should_dump = should_dump;

        auto derived_node_map = perform_derivation(NULL, eval_context);
        if (derived_node_map->find(result_to_return) == derived_node_map->end())
            elog(ERROR, "Didn't find the input result idx!");
        TraceProvData *selected_result = derived_node_map->at(result_to_return);
        traceprov_materialize_derived_result(fcinfo, selected_result);
        return (Datum) 0;
    }

    // Based off pg_prepared_statement.
    static void traceprov_materialize_derived_result(FunctionCallInfo fcinfo, TraceProvData *derived){
        ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
        TupleDesc	tupdesc;
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
        const int32 num_attrs = (int32)derived->size();
        tupdesc = CreateTemplateTupleDesc(num_attrs);
        for (int32 col_idx = 0; col_idx < num_attrs; col_idx++){
            TupleDescInitEntry(tupdesc, (AttrNumber) col_idx + 1, psprintf("column_%d", col_idx), INT8OID, -1, 0);
        }

        tupstore = tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random, false, work_mem);
        rsinfo->returnMode = SFRM_Materialize;
        rsinfo->setResult = tupstore;
        rsinfo->setDesc = tupdesc;
        MemoryContextSwitchTo(oldcontext);

        Datum *record = (Datum *)palloc0(sizeof(Datum)*num_attrs);
        bool *nulls = palloc0_array(bool, num_attrs);
        for (uint64 row_idx = 0; row_idx < derived->at(0)->data->size(); row_idx++){
            for (int32 col_idx = 0; col_idx < num_attrs; col_idx++){
                record[col_idx] = Int64GetDatum(derived->at(col_idx)->data->at(row_idx));
            }
            tuplestore_putvalues(tupstore, tupdesc, record, nulls);
        }
        #if (PG_MAJORVERSION_NUM != 18)
        tuplestore_donestoring(tupstore);
        #endif
        return;
    }

    PG_FUNCTION_INFO_V1(traceprov_dump_derivation);

    Datum traceprov_dump_derivation(PG_FUNCTION_ARGS){
        TraceProvEvaluateNodeContext *eval_context = palloc0_object(TraceProvEvaluateNodeContext);
        eval_context->should_dump = true;
        perform_derivation(NULL, eval_context);
        PG_RETURN_INT64(0);
    }

    PG_FUNCTION_INFO_V1(traceprov_derivation_spec);

    Datum traceprov_derivation_spec(PG_FUNCTION_ARGS){
        TraceProvEvaluateNodeContext *eval_context = palloc0_object(TraceProvEvaluateNodeContext);
        eval_context->should_dump = false;
        char *spec = NULL;
        const auto evaluate_start = std::chrono::high_resolution_clock::now();
        perform_derivation(&spec, eval_context);
        const auto evaluate_end = std::chrono::high_resolution_clock::now();
        const auto duration = evaluate_end - evaluate_start;
        const uint64 duration_time = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '{');
        appendStringInfo(&buf, "\"child_result\": %s,", spec);
        appendStringInfo(&buf, "\"total_time\": %ld", duration_time);
        appendStringInfoChar(&buf, '}');
        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }

    PG_FUNCTION_INFO_V1(traceprov_get_sql_derivation);

    Datum traceprov_get_sql_derivation(PG_FUNCTION_ARGS){
        TraceProvEvaluateNodeContext *eval_context = palloc0_object(TraceProvEvaluateNodeContext);
        eval_context->should_dump = true;
        char *spec = NULL;
        perform_derivation(&spec, eval_context);
        PG_RETURN_TEXT_P(cstring_to_text(spec));
    }

};

