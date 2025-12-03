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

        if ( layer->num_pk_records != list_length(graph->entries)){
            elog(ERROR, "expected to log the count of entries");
        }

        while (ptr_layer_row < ptr_final_row){
            ptr_layer_row = (void *)(((uint64)layer->record_padding) + (uint64)ptr_layer_row);
            bool found = false;
            found = std::binary_search(reference->begin(), reference->end(), ((struct trace_file_forward_row*)ptr_layer_row)->group_count);
            if (found){
                for (int key_idx = 0; key_idx < layer->num_pk_records; key_idx++){
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

        const TraceProvDependency *graph = (TraceProvDependency *)lfirst(list_head(deserializeTraceProvDependency(&parseContext)));
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
                    for (int pk_id = 0; pk_id < bg_trace_layer->num_pk_records + 1; pk_id++)
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
                    for (int key_idx = 0; key_idx < bg_trace_layer->num_pk_records + 1; key_idx++, subq_forward_row_record++){
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

        for (int key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++) filtered_rows[key_idx] = new std::vector<uint64>;

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
                    for (int key_idx = 0; key_idx < bg_trace_layer->num_pk_records; key_idx++){
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
      is_main_worker,
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

        struct local_context *worker_local_context = NULL;

        for (int layer_id = 0; layer_id < TRACEPROV_MAX_LAYER_PER_WORKER; layer_id++){
          
          struct traceprov_aggregate_layer layer = worker_local_context->cached_layers[layer_id];

          if (layer.layer_number == 0) continue;

          const uint32 record_size = layer.record_padding + ( 1 + layer.num_pk_records)*sizeof(int64);

          const uint64 final_ptr_offset = (uint64)get_final_ptr(NULL, &layer);

          if (final_ptr_offset % record_size){
            elog(INFO, "Expected ptr offset to be multiple of record size at (WORKER: %d, LAYER: %d)!", worker_id, layer_id);
          }
          
	  int64 record_count = 0;
      // In some cases it is not defined (like for the group layer files.)
      int32 is_sorted_by_group_no = -1;
	  if (layer.layer_number % 3 == 2) {
	  	// In this case, it is group number. We don't define number of records precisely here.
		// It is actually just whatever the main layer reports as the number of groups.
		// Need -2 because layer numbers are 1-indexed
		record_count = 0;
	  } else {
        const bool is_pure_layer = layer.layer_number % 3 == 1;
        is_sorted_by_group_no = 1;
	  	record_count = final_ptr_offset / record_size;
        // Need to scan over the layer file to determine if it is sorted by group.
        char *layer_mapped_ptr = NULL;
        if (map_layer_file(layer.layer_number, worker_id, (void**)&layer_mapped_ptr, layer.size)){
            elog(ERROR, "Encountered error when mapping the layer for stats");
        }
        const void *layer_final_ptr = get_final_ptr(layer_mapped_ptr, &layer);
        uint64 last_group_number = 0;
        
        while (layer_mapped_ptr < layer_final_ptr){
            layer_mapped_ptr += layer.record_padding;
            const uint64 *typed_ptr = (uint64 *)layer_mapped_ptr;
            uint64 current_group_number = 0;
            if (is_pure_layer){
                current_group_number = typed_ptr[0];
            }else{
                current_group_number = typed_ptr[layer.num_pk_records];
            }
            if (last_group_number == 0){
                last_group_number = current_group_number;
            }
            if (current_group_number != 0 && current_group_number < last_group_number){
                is_sorted_by_group_no = 0;
            }
            layer_mapped_ptr += sizeof(uint64)*(layer.num_pk_records + 1);
            last_group_number = current_group_number;
        }
	  }
          record[TRACEPROV_LAYER_STAT::is_main_worker] = Int32GetDatum(worker_id == context.main_worker_id);
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
        const List *graphs = deserializeTraceProvDependency(&context);
        ListCell *graphCursor;
        StringInfoData buf;
        initStringInfo(&buf);
        std::string graphStr = "{";
        graphStr = graphStr.append("\"graphs\": [");
        bool needsSep = false;
        foreach(graphCursor, graphs){
            if (needsSep){
                graphStr = graphStr.append(",");
            }
            needsSep = true;
            const TraceProvDependency *graph =  (TraceProvDependency *)lfirst(graphCursor);
            const char *graphRepr = traceProvDependencyToJson(graph);
            elog(INFO, "%s", graphRepr);
            graphStr = graphStr.append(graphRepr);
        }
        // End "graphs" key.
        graphStr = graphStr.append("]");
        graphStr = graphStr.append(",");
        graphStr = graphStr.append("\"context\":");
        const char *contextRepr = traceProvParseContextToJson(context);
        elog(INFO, "%s", contextRepr);
        graphStr = graphStr.append(contextRepr);
        graphStr = graphStr.append("}");
        elog(INFO, "%s", graphStr.c_str()); 
        PG_RETURN_TEXT_P(cstring_to_text(graphStr.c_str()));
    }
};

