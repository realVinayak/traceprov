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
    PG_MODULE_MAGIC;
}

extern "C" {
    // static_assert(!(HAVE__BUILTIN_TYPES_COMPATIBLE_P))

    #define UNUSED(X) do {} while(0 && X);

    static std::vector<std::vector<uint64> *> *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const struct local_context *local_context
    );

    int map_layer_file(int layer_number, int worker_id, void **ptr, int file_size){
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

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            struct local_context *worker_local_context = NULL;

            for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++){

                if (worker_local_context->cached_layers[layer_id].layer_number){
                    void *ptr = NULL;
                    map_layer_file(
                        worker_local_context->cached_layers[layer_id].layer_number,
                        worker_id,
                        &ptr,
                        worker_local_context->cached_layers[layer_id].size
                    );
                    char worker_layer[128] = {0};
                    if (ptr == NULL){
                        messages->push_back("Skipping");
                        continue;
                    }
                    sprintf(worker_layer, "(WORKER: %d, Layer: %d)", worker_id, layer_id);
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
                record_count = read_all_columns(layer.layer_number , worker_local_context)->at(0)->size();
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

    // There are two versions because in some (rare-ish) cases multiple keys form the primary key.
    // Usually, that won't happen, so having a separate map is useful to not construct redundant single element vectors
    typedef std::unordered_map<Oid, std::vector<Datum>> TraceProvSingleDerivation;
    typedef std::unordered_map<Oid, std::vector<std::vector<Datum>>> TraceProvMultipleDerivation;

    typedef std::pair<uint8, struct traceprov_aggregate_layer *> TraceProvWorkerLayer;

    typedef std::pair<uint64, uint64> TraceProvColumn;

    enum TraceProvNodeKind {
        T_TP_RELATION,
        T_TP_JOIN,
        T_TP_APPEND
    };

    typedef std::vector<uint64> TraceProvColumnData;
    typedef std::vector<TraceProvColumnData*> TraceProvData;
    typedef std::vector<std::pair<TraceProvColumn*, TraceProvColumn*>*> TraceProvJoinConditions;
    typedef std::vector<uint64> TraceProvOffset;

    typedef struct TraceProvNode {
        TraceProvNodeKind tag;
    } TraceProvNode;

    typedef struct TraceProvRelation {
        TraceProvNodeKind tag;
        TraceProvData *data;
        char *name;
    } TraceProvRelation;

    typedef struct TraceProvJoinExpr {
        TraceProvNodeKind tag;
        TraceProvNode *left;
        TraceProvNode *right;
        // The join condition.
        TraceProvJoinConditions *join_condition;
        std::vector<TraceProvColumn*> *output_columns;
        TraceProvData *result;
        bool is_left_star;
        bool is_right_star;
    } TraceProvJoinExpr;

    typedef struct TraceProvAppend {
        TraceProvNodeKind tag;
        // List of TraceProvNode (get evaulated separately)
        // TraceProvAppend just appends the results individually.
        List *nodes;
    } TraceProvAppender;

    typedef struct TraceProvDerivation {
        TraceProvSingleDerivation single_derivation;
        TraceProvMultipleDerivation multiple_derivation;
        List *derived_join_exprns;
    } TraceProvDerivation;

    typedef std::pair<TraceProvJoinConditions *, TraceProvDependency *> TraceProvSublinkMapInferItem;

    static TraceProvData* traceprov_evaluate_join_exprn(TraceProvJoinExpr *join_exprn);
    static TraceProvData *traceprov_evaluate_node(TraceProvNode *node);
    static TraceProvData *traceprov_evaluate_append(TraceProvAppend *append_node);
    static void traceprov_dump_data_to_csv(TraceProvData *data, char *file_name);
    uint64 traceprov_get_node_column_count(TraceProvNode *node);

    TraceProvRelation *make_traceprov_relation(TraceProvData *data, char *name){
        if (data->size() == 0)
            elog(ERROR, "Expected to have at least 1 column!");
        auto tp_rel = palloc0_object(TraceProvRelation);
        tp_rel->tag = T_TP_RELATION;
        tp_rel->data = data;
        tp_rel->name = name;
        elog(INFO, "REL: %s, %ld;", name, data->at(0)->size());
        return tp_rel;
    }

    TraceProvJoinExpr *make_traceprov_join_expr(
        TraceProvNode *left,
        TraceProvNode *right,
        TraceProvJoinConditions *join_condition,
        std::vector<TraceProvColumn*> *output_columns,
        bool is_left_star = false,
        bool is_right_star = false
    ){
        auto tp_join_exprn = palloc0_object(TraceProvJoinExpr);
        tp_join_exprn->tag = T_TP_JOIN;
        tp_join_exprn->left= left;
        tp_join_exprn->right = right;
        tp_join_exprn->join_condition = join_condition;
        tp_join_exprn->output_columns = output_columns;
        tp_join_exprn->result = nullptr;
        tp_join_exprn->is_left_star = is_left_star;
        tp_join_exprn->is_right_star = is_right_star;
        return tp_join_exprn;
    }

    TraceProvAppend *make_traceprov_append(
        List *nodes
    ){
        auto tp_append = palloc0_object(TraceProvAppend);
        tp_append->tag = T_TP_APPEND;
        if (list_length(nodes) == 0)
            elog(ERROR, "Making append rel with no nodes!");
        tp_append->nodes = nodes;
        return tp_append;
    }

    TraceProvData *make_traceprov_data(std::vector<uint64> *first){
        auto data = new TraceProvData;
        data->push_back(first);
        return data;
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
        auto output_columns = new std::vector<TraceProvColumn*>;
        for (int i = 0; i < output_column_side - 1; i++){
            output_columns->push_back(new TraceProvColumn(2, i + 2));
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
        auto layer_data = read_all_columns(agg_layer_number, local_context);
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
        for (auto offset: *offsets){
            final_column_data->push_back(column_data->at(offset));
        }
    }

    // Gets the data at the input offsets.
    TraceProvData *get_data_at_offsets(TraceProvData *data, std::vector<uint64> *offsets, uint64 skip_idx = 0){
        auto new_data = new TraceProvData;
        for (uint64 column_idx = 1; column_idx <= data->size(); column_idx++){
            auto current_column = new TraceProvColumnData;
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
        std::vector<struct local_context *> *local_contexts,
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
            not_combined_offsets =  new std::vector<uint64>();
            std::vector<uint64> *combined = new std::vector<uint64>();
            for (uint64 offset = 0; offset < match_key->size(); offset++){
                const uint64 key = match_key->at(offset);
                if (TRACEPROV_GET_IS_COMBINED(key)){
                    combined->push_back(TRACEPROV_STRIP_COMBINED(key));
                    combined_offsets->push_back(offset);
                }else{
                    not_combined_offsets->push_back(offset);
                }
            }
            TraceProvData **worker_split = palloc0_array(TraceProvData *, worker_count);
            for (int i = 0; i < worker_count; i++){
                auto worker_vector = new std::vector<std::vector<uint64>*>;
                worker_vector->push_back(new std::vector<uint64>);
                worker_vector->push_back(new std::vector<uint64>);
                worker_split[i] = worker_vector;
            }
            // In this case, need to consider the combine layer too.
            const TraceProvLayerNumber combine_layer_number = layer->combined_aggregate_layer_number;
            if (combine_layer_number == 0)
                elog(ERROR, "Expected combine layer to always be set!");
         
            auto combined_layer_data = read_all_columns(combine_layer_number, local_context);
            // Split the combined layer based on each worker id.
            for (long unsigned int i = 0; i < combined_layer_data->at(0)->size(); i++){
                // Technically, this can be inferred from the local log entry.
                const uint64 current_current_worker_id = combined_layer_data->at(1)->at(i);
                worker_split[current_current_worker_id - 1]->at(0)->push_back(combined_layer_data->at(0)->at(i));
                worker_split[current_current_worker_id - 1]->at(1)->push_back(combined_layer_data->at(2)->at(i));
            }
            // Ignore the data at match key idx (since it needs to be replaced by the stripped offsets.)
            TraceProvData *combined_data_at_offsets = get_data_at_offsets(log_data, combined_offsets, match_key_idx+1);
            combined_data_at_offsets->at(match_key_idx) = combined;
            if (combined_data_at_offsets->at(match_key_idx) != combined)
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
            auto self_logs = read_all_columns(layer_number, local_context);
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
                auto worker_logs = read_all_columns(layer_number, local_contexts->at(i));
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
        // Reserve space for next pointers.
        for (uint32 i = 0; i < current_layer->num_pk_records; i++)
            current_worker_logs->push_back(new std::vector<uint64>);

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
    static std::vector<std::vector<uint64> *> *read_all_columns(
        const TraceProvLayerNumber layer_number,
        const struct local_context *local_context
    ){
        const struct traceprov_aggregate_layer *current_layer = &local_context->cached_layers[layer_number - 1];
        // In this case, the layer wasn't set.
        if (current_layer->layer_number == 0) return nullptr;
        auto data = read_all_columns_simple(current_layer, local_context);

        if (current_layer->rows_layer_number){
            auto row_data = read_all_columns_simple(
                &local_context->cached_layers[current_layer->rows_layer_number - 1],
                local_context
            );
            for (auto second: *row_data){
                data->push_back(second);
            }
        }
        elog(INFO, "READ LAYER: %d, got: %ld", layer_number, data->at(0)->size());
        return data;
    }
    
    static void perform_derive_from_log(
        const TraceProvDependency *log_dependency,
        const uint8 worker_count,
        TraceProvDerivation *derivation,
        TraceProvParseContext *parsed_back_context
    ){
        const TraceProvLayerNumber log_layer_number = log_dependency->headNumber;
        const uint32 layer_width = list_length(log_dependency->entries);

        std::vector<TraceProvWorkerLayer *> *log_layers = new std::vector<TraceProvWorkerLayer *>();
        std::vector<struct local_context *> *worker_local_contexts = new std::vector<struct local_context *>();
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

        elog(INFO, "Found %ld log_layers for %d layer number", log_layers->size(), log_layer_number);
    
        if (list_length(log_dependency->entries) > 64)
            elog(ERROR, "Cannot support more than 64 entries for now...");

        ListCell *entry_cursor = NULL;
        List *next_child_pointers = NIL;
        List *normal_log_entries = NIL;
        foreach(entry_cursor, log_dependency->entries){
            const TraceProvEntry *entry = (TraceProvEntry *)lfirst(entry_cursor);
            if (entry->kind == TP_ENTRY_KIND_POINTER){
                const TraceProvDependency *child_graph = (TraceProvDependency *)list_nth(log_dependency->children, foreach_current_index(entry_cursor));
                if (child_graph == NULL || child_graph->graph_type == TP_INVALID)
                    elog(ERROR, "Got invalid child graph state! Expected to not be null and not to be invalid!");
                next_child_pointers = lappend_int(next_child_pointers, foreach_current_index(entry_cursor));
            } else if (entry->kind == TP_ENTRY_KIND_BASE_RELATION){
                normal_log_entries= lappend_int(normal_log_entries, foreach_current_index(entry_cursor));
            }
        }
        if ((list_length(next_child_pointers) == 0) && (list_length(normal_log_entries) == 0))
            elog(ERROR, "Found both log entries to be completely empty!");

        if ((list_length(next_child_pointers) > 0) && (list_length(normal_log_entries) > 0))
            elog(ERROR, "Found both log entries to be populated... cannot handle this case right now.");

        // If more than 1 final log is found, it implies that final log was parallelized.
        // So, in that case, we don't need to try matching log entries from other workers.
        const bool final_log_was_parallel = log_layers->size() > 1;
        for (auto log_pair: *log_layers){
            const struct traceprov_aggregate_layer *current_layer = log_pair->second;
            TraceProvData *current_worker_logs = read_all_columns(
                current_layer->layer_number,
                worker_local_contexts->at(log_pair->first)
            );

            // Go over all the pointers and split based on each worker.
            ListCell *child_pointer;
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
                derivation->derived_join_exprns = lappend(derivation->derived_join_exprns, traceprov_append_node);
                ListCell *child_entry_cursor;
                auto layer_graph_map = new std::unordered_map<TraceProvLayerNumber, TraceProvSublinkMapInferItem *>;
                foreach(child_entry_cursor, child_graph->entries){
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
                            new TraceProvColumn(1, foreach_current_index(child_entry_cursor) + current_worker_logs->size() + 1),
                            new TraceProvColumn(2, sublink_item->offset_in_key + 1)
                        );
                        item->first->push_back(join_condition);
                    }
                }
                List *sublink_computation_nodes = NIL;
                for(auto layer_graph_map_item: *layer_graph_map){
                    auto child_derivation = new TraceProvDerivation;
                    child_derivation->derived_join_exprns = NIL;
                    TraceProvSublinkMapInferItem *infer_item = layer_graph_map_item.second;
                    TraceProvJoinConditions *join_conditions = infer_item->first;
                    // Derive the graph on the child.
                    perform_derive_from_log(infer_item->second, worker_count, child_derivation, parsed_back_context);
                    TraceProvNode *traceprov_child_append_node = (TraceProvNode *)make_traceprov_append(child_derivation->derived_join_exprns);
                    if (join_conditions->size()){
                        traceprov_child_append_node = (TraceProvNode *)make_traceprov_join_expr(
                            (TraceProvNode*)traceprov_append_node,
                            traceprov_child_append_node,
                            join_conditions,
                            nullptr,
                            true,
                            // So that we don't have to explictly know how it does it.
                            true
                        );
                    }
                    sublink_computation_nodes = lappend(sublink_computation_nodes, traceprov_child_append_node);
                }

                if (list_length(sublink_computation_nodes)){
                    derivation->derived_join_exprns = list_concat(derivation->derived_join_exprns, sublink_computation_nodes);
                }
            }

            if (list_length(normal_log_entries)){
                TraceProvData *cloned_log = new TraceProvData;
                for(auto column: *current_worker_logs){
                    cloned_log->push_back(column);
                }
                // Need to also look at any normal log entries (not pointers.)
                ListCell *normal_log_entry;
                foreach(normal_log_entry, normal_log_entries){
                    cloned_log->push_back(current_worker_logs->at(lfirst_int(normal_log_entry)));
                }
                TraceProvNode *normal_log_relation = (TraceProvNode *)make_traceprov_relation(
                    cloned_log,
                    psprintf("normal_log_%s",  tp_parse_get_unique_alias(parsed_back_context))
                );
                derivation->derived_join_exprns = lappend(derivation->derived_join_exprns, normal_log_relation);
            }
        }
    }

    char *get_sample_values(TraceProvData *data){
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfo(&buf, "[SAMPLE OF %ld]: sample_values: {", data->at(0)->size());
        for (uint64 i = 0; i < Min(data->at(0)->size(), 10); i++){
            appendStringInfo(&buf, "[");
            for (uint64 col = 0; col < data->size(); col++){
                if (col > 0) appendStringInfo(&buf, ",");
                appendStringInfo(&buf, "%ld", data->at(col)->at(i));
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
            appendStringInfo(&buf, "RELATION: %s (columns: %ld, count: %ld)", relation->name, relation->data->size(), relation->data->at(0)->size());
            appendStringInfo(&buf, "sample_values: %s", get_sample_values(relation->data));
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
                appendStringInfo(&buf, " - RESULT_SIZE: [%ld]", join_expr->result->at(0)->size());
                appendStringInfo(&buf, "sample_values: %s", get_sample_values(join_expr->result));
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

    // Evaluates the join exprn
    static TraceProvJoinResult* traceprov_evaluate_join_exprn_key_count_1(
        TraceProvColumnData *left_column,
        TraceProvColumnData *right_column
     ){
        // Basically, return the offsets that are found.
        auto offsets_found = new TraceProvJoinResult;
        auto left_offsets = new TraceProvColumnData;
        auto right_offsets = new TraceProvColumnData;
        offsets_found->push_back(left_offsets);
        offsets_found->push_back(right_offsets);
        std::sort(left_column->begin(), left_column->end());
        for (uint64 offset = 0; offset < right_column->size(); offset++){
            const auto match_key = right_column->at(offset);
            auto it = (std::lower_bound(left_column->begin(), left_column->end(), match_key));
            while (it != left_column->end()){
                if (*it == match_key){
                    auto left_offset = std::distance(left_column->begin(), it);
                    left_offsets->push_back(left_offset);
                    right_offsets->push_back(offset);
                    it++;
                    continue;
                }
                break;
            }
        }
        return offsets_found;
    }

    static std::vector<std::vector<uint64>*> *traceprov_flatten_data(TraceProvData *input_data){
        auto row_count = input_data->at(0)->size();
        auto flattened = new  std::vector<std::vector<uint64>*>;
        for (uint64 row_idx = 0; row_idx < row_count; row_idx++){
            auto row_data = new std::vector<uint64>;
            for (uint64 col_idx = 0; col_idx < input_data->size(); col_idx++){
                row_data->push_back(input_data->at(col_idx)->at(row_idx));
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

    static TraceProvJoinResult *traceprov_evaluate_join_exprn_many_key(
        TraceProvData *left_columns,
        TraceProvData *right_columns
    ){
        auto offsets_found = new TraceProvJoinResult;
        auto left_offsets = new TraceProvColumnData;
        auto right_offsets = new TraceProvColumnData;
        offsets_found->push_back(left_offsets);
        offsets_found->push_back(right_offsets);
        auto left_flattened = traceprov_flatten_data(left_columns);
        auto right_flattened = traceprov_flatten_data(right_columns);
        std::sort(left_flattened->begin(), left_flattened->end(), vector_compare_is_less);

        for (uint64 offset = 0; offset < right_flattened->size(); offset++){
            const auto match_key = right_flattened->at(offset);
            auto it = (std::lower_bound(left_flattened->begin(), left_flattened->end(), match_key, vector_compare_is_less));
            while (it != left_flattened->end()){
                if (vector_compare_is_equal(*it, match_key)){
                    auto left_offset = std::distance(left_flattened->begin(), it);
                    left_offsets->push_back(left_offset);
                    right_offsets->push_back(offset);
                    it++;
                    continue;
                }
                break;
            }
        }
        return offsets_found;
    }

    static TraceProvData* traceprov_evaluate_join_exprn(TraceProvJoinExpr *join_exprn){
        // If this join exprn is already evaluated, simply return the last result.
        if (join_exprn->result != nullptr)
            return join_exprn->result;
        
        TraceProvData *left_result = traceprov_evaluate_node(join_exprn->left);
        TraceProvData *right_result = traceprov_evaluate_node(join_exprn->right);
        // traceprov_dump_data_to_csv(left_columns, psprintf(DEFINE_TRACE_PROV_FILE("/left_data.csv"), DataDir));
        // traceprov_dump_data_to_csv(right_columns, psprintf(DEFINE_TRACE_PROV_FILE("/right_data.csv"), DataDir));
        // This is poor mans index nested loop join.
        // The left result is guaranteed to be smaller than the right side, so it is sorted.

        // Collect all the columns that need to be sorted.
        // This is the common case.
        // Othercases can happen in sublinks.
        TraceProvJoinResult *offsets = nullptr;
        if (join_exprn->join_condition->size() == 1){
            auto join_pair = join_exprn->join_condition->at(0);
            if (join_pair->first->first != 1 || join_pair->second->first != 2)
                elog(ERROR, "Got unexpected numbering..");

            TraceProvColumnData *left_columns =  left_result->at(join_pair->first->second - 1);
            TraceProvColumnData *right_columns =  right_result->at(join_pair->second->second - 1);
            offsets = traceprov_evaluate_join_exprn_key_count_1(
                left_columns,
                right_columns
            );
        }else{
            TraceProvData *left_columns = new TraceProvData;
            TraceProvData *right_columns = new TraceProvData;
            for (auto join_pair: *join_exprn->join_condition){
                if (join_pair->first->first != 1 || join_pair->second->first != 2)
                    elog(ERROR, "Got unexpected numbering...");
                left_columns->push_back(left_result->at(join_pair->first->second - 1));
                right_columns->push_back(right_result->at(join_pair->second->second - 1));
            }
            offsets = traceprov_evaluate_join_exprn_many_key(
                left_columns,
                right_columns
            );
        }
        if (offsets->at(0)->size() == 0)
            elog(ERROR, "Got 0 as the join result!");
        TraceProvData *result = new TraceProvData;
        if(join_exprn->is_left_star){
            auto left_offsets = offsets->at(0);
            for (uint64 i = 0; i < left_result->size(); i++){
                auto filtered_column_result = new TraceProvColumnData;
                get_column_data_at_offset(
                    left_result->at(i),
                    left_offsets,
                    filtered_column_result
                );
                result->push_back(filtered_column_result);
            }
        }
        
        if(join_exprn->is_right_star){
            auto right_offsets = offsets->at(1);
            for (uint64 i = 0; i < right_result->size(); i++){
                auto filtered_column_result = new TraceProvColumnData;
                get_column_data_at_offset(
                    right_result->at(i),
                    right_offsets,
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
                auto filtered_column_result = new TraceProvColumnData;
                auto result_offsets = offsets->at(output_column->first - 1);
                auto data_column = output_column->first == 1 ? left_result : right_result;
                get_column_data_at_offset(
                    data_column->at(output_column->second - 1),
                    result_offsets,
                    filtered_column_result
                );
                result->push_back(filtered_column_result);
            }
        }

        uint64 final_result_size = 0;
        for (uint64 i = 0; i < result->size(); i++){
            if (i == 0){
                final_result_size = result->at(i)->size();
            }else{
                if (final_result_size != result->at(i)->size())
                    elog(ERROR, "Expected final join result to be of same columns: %ld, %ld", final_result_size, result->at(i)->size());
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



    static TraceProvData *traceprov_evaluate_append(TraceProvAppend *append_node){
        ListCell *node_cursor;
        TraceProvNode *first_node = (TraceProvNode *)list_nth(append_node->nodes, 0);
        TraceProvData *result = traceprov_evaluate_node(first_node);
        const uint64 initial_result_width = result->size();
        for_each_from(node_cursor, append_node->nodes, 1){
            TraceProvData *current = traceprov_evaluate_node((TraceProvNode *)lfirst(node_cursor));
            if (current->size() != initial_result_width)
                elog(ERROR, "Expected all the nodes in an append block to have the same width");
            for (uint64 col_cursor = 0; col_cursor < initial_result_width; col_cursor++){
                for (uint64 row_cursor = 0; row_cursor < current->at(col_cursor)->size(); row_cursor++){
                    result->at(col_cursor)->push_back(current->at(col_cursor)->at(row_cursor));
                }
            }
        }
        return result;
    }

    static TraceProvData *traceprov_evaluate_node(TraceProvNode *node){
        if (node->tag == T_TP_RELATION){
            TraceProvRelation *relation = (TraceProvRelation *)node;
            traceprov_dump_data_to_csv(relation->data, psprintf(DEFINE_TRACE_PROV_FILE("/%s.csv"), DataDir, relation->name));
            return relation->data;
        } else if (node->tag == T_TP_JOIN){
            TraceProvJoinExpr *join_exprn = (TraceProvJoinExpr *)node;
            return traceprov_evaluate_join_exprn(join_exprn);
        } else if (node->tag == T_TP_APPEND){
            TraceProvAppend *append_node = (TraceProvAppend *)node;
            return traceprov_evaluate_append(append_node);
        }
        elog(ERROR, "Invalid tag: %d", node->tag);
    }

    static void traceprov_dump_data_to_csv(TraceProvData *data, char *file_name){
        std::string csv_str = "";
        for (uint64 row_idx = 0; row_idx < data->at(0)->size(); row_idx++){
            if (row_idx > 0) csv_str += "\n";
            std::string row_str = "";
            for (uint64 column_idx = 0; column_idx < data->size(); column_idx++){
                if (column_idx > 0) row_str += ",";
                row_str += std::to_string(data->at(column_idx)->at(row_idx));
            }
            csv_str += row_str;
        }
        std::ofstream out(file_name);
        out << csv_str;
        out.close();
    }

    // static derive_from_log(const uint8 worker_count)
    // The main entry point to all the derivation.
    PG_FUNCTION_INFO_V1(traceprov_perform_derivation);

    Datum traceprov_perform_derivation(PG_FUNCTION_ARGS){
        TraceProvParseContext *parsed_back_context = NULL;
        List *graphs = deserializeTraceProvDependency(&parsed_back_context, NULL);
        struct traceprov_shared_context shared_context;

        // NOTE: Shared context is used just to get the total number of workers.
        if (map_traceprov_shared_context(&shared_context))
            elog(ERROR, "Error mapping the shared context");
        
        const uint8 worker_count = shared_context.worker_count;
        ListCell *graph_cursor;
        auto derivation = new TraceProvDerivation;
        derivation->derived_join_exprns = NIL;
        foreach(graph_cursor, graphs){
            TraceProvDependency *graph = (TraceProvDependency *)lfirst(graph_cursor);
            // The top level graph should always be the simple log.
            if (graph->graph_type != TraceProvGraphKind::TP_LOG)
                elog(ERROR, "Expected the top level graph to always be a TP_LOG. Got %d", graph->graph_type);
            perform_derive_from_log(graph, worker_count, derivation, parsed_back_context);
        }
        ListCell *derivation_cursor;
        uint64 final_result_size = 0;
        StringInfoData buf;
        initStringInfo(&buf);
        appendStringInfoChar(&buf, '[');
        foreach(derivation_cursor, derivation->derived_join_exprns){
            uint64 file_idx = foreach_current_index(derivation_cursor) + 1;

            if (file_idx > 1)
                appendStringInfoChar(&buf, ',');

            TraceProvNode *node = (TraceProvNode *)lfirst(derivation_cursor);
            TraceProvData *node_result = traceprov_evaluate_node(node);

            auto dump_file_name = psprintf(DEFINE_TRACE_PROV_FILE("/%ld_dump.csv"), DataDir, file_idx);

            traceprov_dump_data_to_csv(node_result, dump_file_name);

            elog(INFO, "TRACEPROV_EXPRN (COUNT: %ld): %s", node_result->at(0)->size(), traceprov_node_to_string(node));
            elog(INFO, "TRACEPROV_EXPRN (COUNT: %ld)", node_result->at(0)->size());
            final_result_size += node_result->at(0)->size();
            elog(INFO, "Sample result: %s", get_sample_values(node_result));

            appendStringInfo(&buf, "{\"idx\": %ld, \"size\": %ld}", file_idx, node_result->at(0)->size());

        }
        appendStringInfoChar(&buf, ']');
        elog(INFO, "Final result size: %ld", final_result_size);
        PG_RETURN_TEXT_P(cstring_to_text(buf.data));
    }
};

