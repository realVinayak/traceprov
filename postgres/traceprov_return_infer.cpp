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
#include <list>
#undef HAVE__BUILTIN_TYPES_COMPATIBLE_P

// #ifndef TRACEPROV_PAGE_SIZE
// #define TRACEPROV_PAGE_SIZE 0
// #endif

extern "C" {
    #include "postgres.h"
    #include "funcapi.h"
    #include "fmgr.h"
    #include "miscadmin.h"
    #include "traceprov.h"
    #include "file_utils.h"
    #include "utils/builtins.h"
    PG_MODULE_MAGIC;

    #define TRACEPROV_OTIMES    " ⊗ "
    #define TRACEPROV_PLUS      " ⊕ "
    #define TRACEPROV_SOMETHING "𝟙"
    #define TRACEPROV_DELTA     "δ"
}

extern "C" {

    #define UNUSED(X) do {} while(0 && X);

    #define CACHED_LAYERS 24

    struct mmap_entry {
        void *ptr;
        size_t size;
    };

    std::list<struct mmap_entry> *mmap_entries = NULL;

    static inline void initialize_mmap_entries(){
        if (mmap_entries == NULL){
            mmap_entries = new std::list<struct mmap_entry>;
        }
        if (mmap_entries == NULL){
            elog(ERROR, "Error making the mmap entries");
        }
    }

    static inline void cleanup_mmap(){
        for (struct mmap_entry &entry: *mmap_entries){
            munmap(entry.ptr, entry.size);
        };
        mmap_entries = NULL;
    }

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

        struct mmap_entry entry = {
            .ptr = temp_ptr,
            .size = file_size * TRACEPROV_PAGE_SIZE
        };

        mmap_entries->push_back(entry);
        return 0;
    }

    int map_traceprov_shared_context(struct traceprov_shared_context *ptr){
        const size_t size_shared_context_filename = sizeof(TRACEPROV_SHARED_CONTEXT) + strlen(DataDir) + 1;
        struct mmap_entry entry;
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

        entry = {
            .ptr = temp_ptr,
            .size = TRACEPROV_SHARED_CONTEXT_SIZE
        };

        mmap_entries->push_back(entry);
    
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
        std::vector<int64> **ids;
    };

    void store_inference(
        Tuplestorestate *tupstore,
        TupleDesc tupdesc,
        const struct infer_result *result
    ){
        const size_t width = result->width;
        std::vector<int64> ** pk_records = result->ids;
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

    std::vector<int64> *set_diff(std::vector<int64> *first, std::vector<int64> *second){
      // set diff, assumes sorted.
      unsigned long int iter_first = 0;
      unsigned long int iter_second = 0;
      std::vector<int64> *set_diff_computed = new std::vector<int64>;
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

    struct infer_result * perform_inference(
        const unsigned int layer_number,
        const unsigned int reference_layer, 
        const unsigned int subq_layer_number,
        // This is for generating polynomials.
        // Basically, when this is done, it'll perform inference _just_ for this group.
        // TODO: Handle nested groups for polynomial generation?
        const uint64 final_group_pointer_ref
        ){

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context)){
            elog(ERROR, "Error mmaping shared context");
        }


        const int group_layer_number = layer_number + 1;
        const int partial_group_ln = layer_number + 2;

        const struct local_context *main_worker_context = &context.local_contexts[context.main_worker_id];
        const struct traceprov_aggregate_layer *main_trace_layer = &main_worker_context->cached_layers[layer_number - 1];
        const struct traceprov_aggregate_layer *group_layer = &main_worker_context->cached_layers[group_layer_number - 1];
        const struct traceprov_aggregate_layer *partial_group_layer = &main_worker_context->cached_layers[partial_group_ln - 1];

        auto present_groups = new std::vector<int64>;
        if (subq_layer_number){
            std::vector<int64> ** subq_records = NULL;
            int subq_width = 0;
            // Here, it is entirely possible that the subquery gets parallelized.
            // So, we'd have to look at all the workers.
            const int subq_index = subq_layer_number - 1;
            for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

                const struct local_context *bg_context = &context.local_contexts[worker_id];
                const struct traceprov_aggregate_layer *bg_trace_layer = &bg_context->cached_layers[subq_index];

                if (bg_trace_layer->layer_number != subq_layer_number) continue;
                
                subq_width = bg_trace_layer->num_pk_records + 1;
                if (subq_records == NULL){
                    // Need to have +1 because of the adjusting that was done during tracing.
                    subq_records = (std::vector<int64> **)malloc(sizeof(std::vector<int64> *)*(bg_trace_layer->num_pk_records + 1));
                    for (int pk_id = 0; pk_id < bg_trace_layer->num_pk_records + 1; pk_id++)
                        subq_records[pk_id] = new std::vector<int64>;
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

        std::vector<int64> * groups_to_filter = new std::vector<int64>;
        
        if (reference_layer){
            const struct traceprov_aggregate_layer *group_reference_layer = &main_worker_context->cached_layers[reference_layer];
            void *group_reference_ptr = NULL;
            if (map_layer_file(reference_layer + 1, context.main_worker_id, &group_reference_ptr, group_reference_layer->size)){
                PRINT_ON_DEBUG("Error opening group reference layer");
                elog(ERROR, "Error opening group reference layer");
            }
            for (int64 reference_group_idx = 0; reference_group_idx < group_layer->num_groups; reference_group_idx++){
                const struct trace_file_grouped_row *gr = &((struct trace_file_grouped_row *)group_reference_ptr)[reference_group_idx];
                if (gr->in_result){
                    groups_to_filter->push_back(reference_group_idx + 1);
                }
            }

            std::sort(groups_to_filter->begin(), groups_to_filter->end());
        }


        if (final_group_pointer_ref == 0){
            for (int64 group_idx = 0; group_idx < main_trace_layer->num_groups; group_idx++){
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
        }else{
    
            const int final_region_number = TRACEPROV_NUM_REGIONS_GROUP(group_layer->size);

            // We need to be quick here (because this path will be taken for each of the group)
            // To do so, we use a mask.
            // Need to be careful here. The mask depends on which region we are checking.
            int region_number = 0;
            for (region_number = 0; region_number < final_region_number; region_number++){

                const uint64 region_start = ((uint64*)group_layer->current_row)[region_number];
                const uint64 region_end = region_number == 0 ? (uint64)((char*)(void*)region_start + TRACEPROV_PAGE_SIZE) : (uint64)((char*)(void*)region_start + (TRACEPROV_INCREMENT_GROUP_BY_PG*TRACEPROV_PAGE_SIZE));

                if (final_group_pointer_ref >= region_start && final_group_pointer_ref < region_end){
                    // We've now found the region where the page belongs too.
                    // From here, we can compute the group number.
                    // First, need to compute how many groups were before us. This is done by computing the number of pages, and dividing it by single group size.
                    // This is written like below to improve readibility.
                    const uint64 pages_behind = (region_number == 0) ? 0 : ((region_number == 1 ? 1 : (1 + (region_number - 1)*TRACEPROV_INCREMENT_GROUP_BY_PG)));
                    const uint64 group_size = ((group_layer->num_pk_records + 1)*sizeof(int64));
                    const uint64 groups_behind = (pages_behind * TRACEPROV_PAGE_SIZE) / group_size;
                    const uint64 group_index_within_range = (((uint64)final_group_pointer_ref - (uint64)((((void**)(group_layer->current_row))[region_number]))) / group_size) + 1;
                    const uint64 final_group_number = group_index_within_range + groups_behind;

                    present_groups->push_back(final_group_number);
                    break;
                }
            }
            
            if (present_groups->size() == 0){
                assert(0);
                elog(ERROR, "Didn't find the region!");
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
        
        std::vector<int64> ** groups_per_worker = (std::vector<int64> **)malloc(sizeof(std::vector<int64>*)*(context.worker_count));
        for (int worker_id = 0; worker_id < context.worker_count; worker_id++) groups_per_worker[worker_id] = new std::vector<int64>;

        std::vector<int64> *present_groups_found = new std::vector<int64>;

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

        std::vector<int64> ** filtered_rows = (std::vector<int64> **)malloc(sizeof(std::vector<int64> *)*(main_trace_layer->num_pk_records));

        for (int key_idx = 0; key_idx < main_trace_layer->num_pk_records; key_idx++) filtered_rows[key_idx] = new std::vector<int64>;

        int iters_made = 0;
        std::vector<int64>* main_worker_set_difference = nullptr;

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            std::vector<int64> *local_group_nos = groups_per_worker[worker_id];
            // In case of main worker, we can be in the case where the group was completely within our portion of the table
            // In that case, we'd miss logging it in the local_group_nos.
            if (local_group_nos->size() == 0 && worker_id != context.main_worker_id) continue;

            std::sort(local_group_nos->begin(), local_group_nos->end());

            const struct local_context *bg_context = &context.local_contexts[worker_id];
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

        initialize_mmap_entries();

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

        struct infer_result *infer_result_computed = perform_inference(layer_number, reference_layer, subq_layer_number, 0);
        store_inference(tupstore, tupdesc, infer_result_computed);
        cleanup_mmap();

        return (Datum) 0;
    }

    PG_FUNCTION_INFO_V1(traceprov_infer_time);

    // Runs the inference, and returns just the time taken to complete the inference.
    // Note that it is the time to just fill-up the buffer with primary keys.
    // So, it is a good indicator of overhead of inference (rather than materialization)
    Datum traceprov_infer_time(FunctionCallInfo fcinfo){
        
        const int32 layer_number = PG_GETARG_INT32(0);
        const int32 reference_layer = PG_GETARG_INT32(1);
        const int32 subq_layer_number = PG_GETARG_INT32(2);

        initialize_mmap_entries();
        auto start = std::chrono::high_resolution_clock::now();

        struct infer_result *infer_result_computed = perform_inference(layer_number, reference_layer, subq_layer_number, 0);

        auto end = std::chrono::high_resolution_clock::now();
        
        UNUSED(infer_result_computed);

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        uint64 duration_time = (uint64)duration.count();

        cleanup_mmap();
        PG_RETURN_INT64(duration_time);
    }

    PG_FUNCTION_INFO_V1(traceprov_sync_time);

    Datum traceprov_sync_time(FunctionCallInfo fcinfo){

        initialize_mmap_entries();

        std::vector<std::string> *messages = new std::vector<std::string>;

        auto start = std::chrono::high_resolution_clock::now();

        struct traceprov_shared_context context;
        if (map_traceprov_shared_context(&context)){
            elog(ERROR, "Error mmaping the shared context for sync!");
        }

	    int final_code = 0;

        for (int worker_id = 0; worker_id < context.worker_count; worker_id++){

            struct local_context *worker_local_context = &context.local_contexts[worker_id];

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
        cleanup_mmap();
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

      initialize_mmap_entries();
        
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

        struct local_context *worker_local_context = &context.local_contexts[worker_id];

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
		record_count = context.local_contexts[context.main_worker_id].cached_layers[layer.layer_number - 2].num_groups;
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
      cleanup_mmap();
      return (Datum) 0;
    }

    PG_FUNCTION_INFO_V1(traceprov_infer_poly);

    Datum traceprov_infer_poly(PG_FUNCTION_ARGS){
        StringInfoData buf;
        initStringInfo(&buf);

        initialize_mmap_entries();

        const int32 layer_number = PG_GETARG_INT32(0);
        const uint64 reference_ptr = (uint64)(PG_GETARG_INT64(1));
        const int32 width = (uint64)(PG_GETARG_INT32(2));

        const int32 idx_present = (uint64)(PG_GETARG_INT32(3));
        const int32 idx_in_poly = (uint64)(PG_GETARG_INT32(4));
        struct infer_result *infer_result_computed = perform_inference(
            layer_number,
            0,
            0,
            reference_ptr
        );

        cleanup_mmap();
        std::string tuple_repr = "";
        bool needs_outer_sep = false;
        for (int record_num = 0; record_num < infer_result_computed->ids[0]->size(); record_num++){
            bool needs_sep = false;
            std::string row_repr = "";
            for (int key_id = 0; key_id < width; key_id++){
                if (needs_sep){
                    row_repr += TRACEPROV_OTIMES;
                }
                needs_sep = true;
                std::string value = TRACEPROV_SOMETHING;
                if (key_id == idx_in_poly){
                    value = std::to_string(infer_result_computed->ids[idx_present]->at(record_num));
                }
                row_repr += value;
            }
            if (width > 1){
                row_repr = "(" + row_repr + ")";
            }
            if (needs_outer_sep){
                tuple_repr += TRACEPROV_PLUS;
            }
            needs_outer_sep = true;
            tuple_repr += row_repr;
        }

        std::string final_repr ="";
        final_repr.append(TRACEPROV_DELTA);
        final_repr.append("((");
        final_repr.append(tuple_repr);
        final_repr.append("))");

        PG_RETURN_TEXT_P(cstring_to_text(final_repr.c_str()));
    }

};

