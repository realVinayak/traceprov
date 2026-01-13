#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>

#include "traceprov.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "file_utils.h"
#include "traceprov_utils.h"
#include <lib/stringinfo.h>
#include "libpq/pqformat.h"
#include "common/file_perm.h"

#include "nodes/execnodes.h"

#if (PG_MAJORVERSION_NUM >= 16)
#include "varatt.h"
#endif

PG_MODULE_MAGIC;

int grow_group_page_mapping(const int, struct traceprov_aggregate_layer *);
int grow_layer_file(struct traceprov_aggregate_layer *);

static const int32 traceprov_shared_context_magic = 0xBADB00DE;

static struct current_context traceprov_current = {
    .my_worker_id =                 0,
    .traceprov_shared_context_fd =  -1,
    .shared_context =               NULL,
    .local_context =                NULL,
    .maximum_local_layer_used =     0
};

static inline void grow_if_full(struct traceprov_aggregate_layer *layer){
    if (unlikely(layer->current_row == layer->end_of_memory_zone)){
        if (unlikely(grow_layer_file(layer))){
            elog(ERROR, "Received an error when growing trace file.");
        }
    }
}

#define TRACEPROV_GROW_IF_TRUE(layer, cond) do { \
    if (unlikely(cond)) { \
        grow_layer_file(layer); \
    } \
} while(0); \

static inline void grow_if_full_bytes(struct traceprov_aggregate_layer *layer, const size_t bytes){
    if (unlikely(
            (layer->current_row == layer->end_of_memory_zone)
        || (layer->current_row == (layer->end_of_memory_zone - bytes))
        )
    ){
        if (unlikely(grow_layer_file(layer))){
            elog(ERROR, "Received an error when growing trace file.");
        }
    }
}

static inline int round_up(const int number){
    return number == 1 ? 1 : (1 << (64 - __builtin_clzl(number - 1)));
}

static int initialize_file(int fd, const int32 *magic_word, size_t size){
    int rc = 0;
    // First, truncate the file to 0.
    if ((rc = ftruncate(fd, 0))){
        elog(INFO, "Error truncating file to 0.");
        goto out;
    }
    // Truncate to size.
    if ((rc = ftruncate(fd, size))){
        elog(INFO, "Error truncating shared context file to shared context size..");
        goto out;
    }

    off_t moved = lseek(fd, 0, SEEK_SET);
    if (moved == -1){
        rc = 1;
        elog(INFO, "Error doing lseek to beginning on shared context file");
        goto out;
    }
    // Write the magic word.
    if (magic_word != NULL){
        if(write(fd, magic_word, sizeof(int32)) != sizeof(int32)){
            rc = 1;
            elog(INFO, "Error writing required amount;");
            goto out;
        }
    }
out:
    return rc;
}

static int fail_safe_mmap(int fd, size_t size, void **pptr){
    void *ptr = mmap(
        NULL,
        size,
        PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
    if (ptr == MAP_FAILED){
        elog(INFO, "Error mmaping the file, initially");
        return 1;
    }
    *pptr = ptr;
    return 0;
}

static int initialize_local_context(){
    
    if (traceprov_current.my_worker_id != 0) return 0;

    int rc = 0, is_locked = 0, shared_context_fd = 0, worker_layer_map_fd = 0;
    char *shared_context_file_name = psprintf(TRACEPROV_SHARED_CONTEXT, DataDir);

    PRINT_ON_DEBUG("Using %s as shared dir.", shared_context_file_name);

    shared_context_fd = open(shared_context_file_name, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);

    if (shared_context_fd < 0){
        PRINT_ON_DEBUG("Error opening shared context file.");
        rc = 1;
        goto exit_initialize_local_context;
    }

    PRINT_ON_DEBUG("Opened shared context file correctly");

    if ((rc = flock(shared_context_fd, LOCK_EX))){
        PRINT_ON_DEBUG("Error locking the file. %d", rc);
        goto exit_initialize_local_context;
    }

    PRINT_ON_DEBUG("Locked share context file");

    is_locked = 1;

    int32 magic_word = 0;
    uint32 maximum_layer_used = 0;

    if(read(shared_context_fd, &magic_word, sizeof(int32)) == -1){
        PRINT_ON_DEBUG("Had error reading in magic word.");
        goto exit_initialize_local_context;
    }

    if (magic_word != traceprov_shared_context_magic){
        // The magic word didn't match. Need to initialize the file.
        if ((rc = initialize_file(shared_context_fd, &traceprov_shared_context_magic, TRACEPROV_SHARED_CONTEXT_SIZE))){
            goto exit_initialize_local_context;
        }
        int graph_file_fd = open(psprintf(TRACEPROV_GRAPH_FILE, DataDir), O_RDWR);
        if (graph_file_fd < 0){
            elog(INFO, "Error opening the graph file!");   
        }else{
            if((read(graph_file_fd, &maximum_layer_used, sizeof(uint32))) == -1){
                elog(ERROR, "Read from graph file failed!");
            }
            if(close(graph_file_fd)) elog(ERROR, "Error closing graph file");   
        }
    }

    PRINT_ON_DEBUG("Mmaping the shared context file.");

    struct traceprov_shared_context *shared_context = NULL;
    
    if ((rc = fail_safe_mmap(shared_context_fd, TRACEPROV_SHARED_CONTEXT_SIZE, (void*)&shared_context))){
        goto exit_initialize_local_context;
    }

    // There is now an exclusive lock on the shared context file (so we can increment easily.)
    traceprov_current.my_worker_id = (++shared_context->worker_count);
    if (maximum_layer_used > 0){
        shared_context->maximum_layer_number_used = maximum_layer_used;
        traceprov_current.maximum_local_layer_used = maximum_layer_used;
    }else{
        traceprov_current.maximum_local_layer_used = shared_context->maximum_layer_number_used;
    }

    if (traceprov_current.my_worker_id >= TRACEPROV_MAX_WORKERS){
        elog(ERROR, "Maximum worker count reached.");
        rc = 1;
        goto exit_initialize_local_context;
    }

    worker_layer_map_fd = open(psprintf(TRACEPROV_WORKER_LAYER_MAP, DataDir, traceprov_current.my_worker_id), O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);
    if (worker_layer_map_fd < 0){
        elog(INFO, "Error maping worker layer fd");
        goto exit_initialize_local_context;
    }
    if ((rc = initialize_file(worker_layer_map_fd, NULL, sizeof(struct local_context)))){
        goto exit_initialize_local_context;
    }
    // Set up the local context in a file.
    // This makes everything guaranteed to be on a different page.
    if ((rc = fail_safe_mmap(worker_layer_map_fd, sizeof(struct local_context), (void*)&traceprov_current.local_context))){
        goto exit_initialize_local_context;
    }
    // Set up the current context. All this is local (so, not visible to other processes.)
    traceprov_current.traceprov_shared_context_fd = shared_context_fd;
    traceprov_current.shared_context = shared_context;

    // Initialize the local context (in shared)
    traceprov_current.local_context->worker_id = traceprov_current.my_worker_id;
    traceprov_current.local_context->worker_pid = MyProcPid;

    #if (PG_MAJORVERSION_NUM <= 16)
    if (!IsBackgroundWorker){
        shared_context->main_worker_id = traceprov_current.my_worker_id;
    }
    #else
    if (!AmBackgroundWorkerProcess()){
        shared_context->main_worker_id = traceprov_current.my_worker_id;
    }
    #endif

    if (rc || !is_locked){
        elog(ERROR, "Expected rc to be 0, and the shared context file to be locked.");
    }

exit_initialize_local_context:
    int previous_error = rc;
    if (is_locked){
        if ((rc = flock(shared_context_fd, LOCK_UN))){
            PRINT_ON_DEBUG("Error unlocked share context file: %d", rc);
            return previous_error;
        }
    }
    // Don't need to keep the shared context in-memory too.
    if (shared_context_fd > 0) close(shared_context_fd);
    // Don't also need to keep the worker layer map file open.
    if (worker_layer_map_fd > 0) close(worker_layer_map_fd);
    return rc;
}

static int get_or_create_layer(
    uint32 layer_number,
    struct traceprov_aggregate_layer **p_layer,
    uint32 record_width,
    bool set_current_row,
    bool *is_already_present
){
    struct traceprov_aggregate_layer *layer = NULL;
    int rc = 0;

    // This is the fast path, if the layer per worker fits in the cached layers.
    if (layer_number < TRACEPROV_MAX_LAYER_PER_WORKER){
        layer = &traceprov_current.local_context->cached_layers[layer_number - 1];
    }else{
        int layer_fd = 0;
        const int relative_index = layer_number - TRACEPROV_MAX_LAYER_PER_WORKER;

        // Check if we've already opened the layer file before.
        // If we haven't, need to now open it.
        if (traceprov_current.local_context->layer_fd){
            layer_fd = traceprov_current.local_context->layer_fd;
            layer = traceprov_current.local_context->layers;
        }else{
            // Now, need to do dynamic memory allocation
            char *layer_file_name = psprintf(TRACEPROV_PER_WORKER_FILE,  DataDir, traceprov_current.my_worker_id);
            if (layer_file_name == NULL){
                return 1;
            }

            if (traceprov_current.local_context->dynamic_layer_count == 0){
                // Need to initialize the worker's layer file
                layer_fd = remove_and_create(layer_file_name, TRACEPROV_PAGE_SIZE);
                if (layer_fd < 0){
                    return 1;
                }


                layer = (struct traceprov_aggregate_layer *) mmap(
                    NULL,
                    TRACEPROV_PAGE_SIZE,
                    PROT_WRITE,
                    MAP_SHARED,
                    layer_fd,
                    0
                );

                if (layer == MAP_FAILED){
                    close(layer_fd);
                    return 1;
                }

                // Here, since the file is just created, this is the maximum number of layers
                // we can store.
                traceprov_current.local_context->dynamic_layer_count = TRACRPROV_NUM_LAYER_PER_PAGE;
                traceprov_current.local_context->layers = layer;
                traceprov_current.local_context->layer_fd = layer_fd;
            }
        }

        // Now, we check if the layer is actually big enough to hold the incoming layer number.
        // It is _entirely_ possible that the layer numbers are not contiguous.
        // This is because postgres might decide to reorder functions.
        // TODO: Check if we can implement some kind of "hole" system here.
        if (relative_index > traceprov_current.local_context->dynamic_layer_count){
            // Need to expand the layers.
            // Currently, the most simple way, is to truncate the file, unamp it and re-map it.
            
            // Compute the number of pages that'll hold the layer.
            // The pages are doubled to avoid reallocating soon.
            const int final_page_numbers = (relative_index / TRACRPROV_NUM_LAYER_PER_PAGE) * 2;
            const int final_size = final_page_numbers * TRACEPROV_PAGE_SIZE;
            if ((rc = ftruncate(layer_fd,  final_size))){
                PRINT_ON_DEBUG("Error retruncating the file.");
                return rc;
            }

            rc = munmap(layer, TRACEPROV_PAGE_SIZE * (traceprov_current.local_context->dynamic_layer_count / TRACRPROV_NUM_LAYER_PER_PAGE));
            if (rc){
                PRINT_ON_DEBUG("Error unmaping the file.");
                elog(ERROR, "Error unmaping the file, beyond truncation");
                return rc;
            }

            void *new_map = mmap(NULL, final_size, PROT_WRITE, MAP_SHARED, layer_fd, 0);
            if (new_map == MAP_FAILED){
                elog(ERROR, "Error re-mapping the file, after truncating");
                return rc;
            }

            layer = (struct traceprov_aggregate_layer *) new_map;
            traceprov_current.local_context->layers = layer;
            traceprov_current.local_context->dynamic_layer_count = final_page_numbers / TRACRPROV_NUM_LAYER_PER_PAGE;
        }

        // Here, relative index is used (rather than layer number, which is greater than TRACEPROV_MAX_LAYER_PER_WORKER)
        layer = &layer[relative_index];
    }

    // Finally, we check if the layer number has been set.
    // This helps us determine if we've to actually do the work of mmaping the file.
    if (layer->layer_number != 0){
        *is_already_present = true;
        return 0;
    }

    // Map the actual trace file for this layer.
    // Each worker gets its own trace file.
    int trace_file_fd = remove_and_create(
        psprintf(TRACEPROV_MAIN_TRACE_FILE, DataDir, layer_number, traceprov_current.my_worker_id),
        TRACEPROV_PAGE_SIZE
    );

    if (trace_file_fd < 0){
        return 1;
    }
    
    // Need to mmap the file.
    void *trace_ptr = mmap(
        NULL,
        TRACEPROV_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        trace_file_fd,
        0
    );

    if (trace_ptr == MAP_FAILED){
        PRINT_ON_DEBUG("Mapping the trace file failed.");
        return 1;
    }

    layer->num_pk_records = record_width;
    layer->last_mapping = trace_ptr;
    // This is skipped because it is redundantly 0, but adding it here for documentation.
    // layer->mapping_count = 0;
    if (set_current_row) layer->current_row = trace_ptr;
    // layer->num_groups = 0;
    layer->layer_number = layer_number;
    
    const int total_record_size = ((record_width) * sizeof(int64));
    layer->record_padding = round_up(total_record_size) - total_record_size;
    if (layer->record_padding < 0){
        elog(ERROR, "Found invalid padding");
        return 1;
    }
    layer->end_of_memory_zone = TRACEPROV_PAGE_SIZE + trace_ptr;
    layer->layer_fd = trace_file_fd;
    layer->size = 1;
    if (p_layer) *p_layer = layer;
    return rc;
}

static int initialize_layer_file(
    const uint32 layer_number,
    // Specifies the length of the key of the record.
    const uint32 key_length,
    // Specifies the length of the record, excluding keys.
    const uint32 record_length,
    const bool set_current_row, 
    const Node *state
){

    /**
     * Here are the sequence of operations this function needs to perform.
     * 1. Make space in local_context's layers for the current layer (or reuse previous space)
     * 2. Create the trace file for this layer
     */

    struct traceprov_aggregate_layer *layer = NULL;
    int rc = 0;
    bool is_already_present = false;

    if ((rc = get_or_create_layer(layer_number, &layer, key_length, set_current_row, &is_already_present))){
        elog(ERROR, "Error creating the layer file (column)");
        return rc;
    }

    if (is_already_present) return 0;

    uint32 record_layer_number = 0;
    if (record_length > 0 && layer->rows_layer_number == 0){
        record_layer_number = ++traceprov_current.maximum_local_layer_used;
        if ((rc = get_or_create_layer(record_layer_number, NULL, record_length, set_current_row, &is_already_present))){
            elog(ERROR, "Error creating the layer file (key)");
            return rc;
        }
        layer->rows_layer_number = record_layer_number;
    }

    if (state != NULL){
        // If the entries in this file will be hashed, need to also make the hash buckets for them.
        // This effectively makes a recursive call (but the state of the next is always null)
        // Technically, the recursive call can be used to implement a multi-level partitioning...
        if (TRACEPROV_SHOULD_HASH(state) && layer->buckets[0] == 0){
            // Need to make the new levels
            // Note that we only need to construct buckets after the current layer.
            // because the current layer acts as the buckets for the other ones..
            for (int bucket_idx = 0; bucket_idx < TRACEPROV_BUCKET_COUNT - 1; bucket_idx++){
                const uint32 hash_bucket_layer_number = ++traceprov_current.maximum_local_layer_used;
                layer->buckets[bucket_idx] = hash_bucket_layer_number;
                if (initialize_layer_file(hash_bucket_layer_number, key_length, record_length, true, NULL)){
                    elog(ERROR, "Error initializing the hash buckets");
                }
            }
        }
        if (IsA(state, AggState)){
            layer->aggregate_strategy = ((AggState *)state)->aggstrategy;
        }
    }
    return 0;
}

static int initialize_local_and_layer(
    const uint32 layer_number,
    const uint32 key_length,
    const uint32 record_length,
    const bool set_current_row,
    const Node *state
){
    int rc = 0;
    if ((rc = initialize_local_context())){
        PRINT_ON_DEBUG("Error initializing local context.");
        return rc;
    }
    if ((rc = initialize_layer_file(
        layer_number,
        key_length,
        record_length,
        set_current_row,
        state
    ))){
        PRINT_ON_DEBUG("Error initializing layer file");
    }
    return rc;
}

// Assumes layer has already been created.
static inline struct traceprov_aggregate_layer *get_layer(const uint32 layer_number){
    if (layer_number == 0){
        elog(ERROR, "Expected to always be called with layer number > 0");
    }
    if (layer_number < TRACEPROV_MAX_LAYER_PER_WORKER){
        return &traceprov_current.local_context->cached_layers[layer_number - 1];
    }
    return &traceprov_current.local_context->layers[layer_number - TRACEPROV_MAX_LAYER_PER_WORKER];
}

PG_FUNCTION_INFO_V1(test_local_setup);

Datum test_local_setup(PG_FUNCTION_ARGS){
    int rc = initialize_local_context();
    if (rc) PG_RETURN_INT32(rc);
    rc = initialize_layer_file(PG_GETARG_INT32(0), 1, PG_GETARG_INT32(1), true, NULL);
    print_layer(get_layer(PG_GETARG_INT32(0)));
    PG_RETURN_INT32(rc);
}

PG_FUNCTION_INFO_V1(reinit_state);

Datum reinit_state(PG_FUNCTION_ARGS){
    traceprov_current.my_worker_id = 0;
    traceprov_current.traceprov_shared_context_fd  = -1;
    traceprov_current.shared_context = NULL;
    traceprov_current.local_context = NULL;
    traceprov_current.maximum_local_layer_used = 0;
    char *traceprov_data_dir = psprintf(TRACE_PROV_DIR, DataDir);
    PRINT_ON_DEBUG("TRACEPROV_DIR: %s", traceprov_data_dir);

    int rc = create_dir_if_not_exists(traceprov_data_dir, pg_dir_create_mode);
    if (rc){
        PRINT_ON_DEBUG("Error creating dir: %d", rc);
        PG_RETURN_INT32(rc);
    }
    rc = remove_files_from_dir(traceprov_data_dir);
    if (rc){
        PRINT_ON_DEBUG("Error removing files: %d", rc);
    }

    PG_RETURN_INT32(rc);    
}

static void inline append_sorted_column(
    struct traceprov_aggregate_layer *rows_layer,
    struct traceprov_aggregate_layer *column_layer
){
    grow_if_full(column_layer);
    const uint64 hole_size = (rows_layer->current_row - rows_layer->last_mapping) + (rows_layer->size - 1)*TRACEPROV_PAGE_SIZE;
    *((uint64*)column_layer->current_row) = (hole_size / TRACEPROV_GET_RECORD_SIZE(rows_layer));
    column_layer->current_row += sizeof(uint64);
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_sfunc);

Datum traceprov_agg_key_sfunc(PG_FUNCTION_ARGS){

    int rc = 0;
    // Argument 0 is the internal state.
    const uint32 layer_number = PG_GETARG_UINT32(1);
    const uint32 record_length = PG_NARGS() - 2; // 1 for layer number, 1 for internal state.
    const bool is_init = PG_ARGISNULL(0);

    struct traceprov_agg_context *agg_context;
    MemoryContext agg_mem_context;
    if (!AggCheckCallContext(fcinfo, &agg_mem_context))
        elog(ERROR, "aggregate function called in non-aggregate context");

    if ((rc = initialize_local_and_layer(layer_number, 1, record_length, true, (Node *)fcinfo->context))){
        PRINT_ON_DEBUG("Error setting up local or layer");
        elog(ERROR, "Error setting up local or layer");
        return 1;
    }

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);

    // The bucket zero just means the current layer.
    uint8 bucket = 0;

    if (is_init){
        const uint64 absolute_group_number =  ++main_layer->num_groups;
        agg_context = (struct traceprov_agg_context *)MemoryContextAlloc(agg_mem_context, sizeof(struct traceprov_agg_context));
        // If we determine that we're going to hash this, we store the hash bucket in the second byte of group count field.
        // This helps speeding things up during inference, and we don't have to worry about hashing it the next time this gets
        // called. Also, during inference, the value is not hashed again.
        if (TRACEPROV_SHOULD_HASH(fcinfo->context)){
            bucket = (traceprov_hashint8(absolute_group_number)) % TRACEPROV_BUCKET_COUNT;
        }
        // Also set the worker id here.
        // This is done because, technically, postgres can apply optimizations where it finalizes an aggregate entirely
        // if it lives inside of a partition. In the worst case, that happens on a remote process (so need to infer it back by also logging worker id)
        agg_context->group_cnt = TRACEPROV_SET_WORKER_ID(TRACEPROV_SET_BUCKET(absolute_group_number, bucket), traceprov_current.my_worker_id);
        agg_context->worker_id = traceprov_current.my_worker_id;
        agg_context->is_combined = 0;
        agg_context->layer_number = layer_number;
    }else{
        agg_context = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
        if (TRACEPROV_SHOULD_HASH(fcinfo->context)){
            bucket = TRACEPROV_GET_BUCKET(agg_context->group_cnt);
        }
    }

    struct traceprov_aggregate_layer *current_column_layer = NULL;
    if (bucket == 0){
        current_column_layer = main_layer;
    }else{
        // bucket always > 0 at this point.
        current_column_layer = get_layer(main_layer->buckets[bucket - 1]);
    }

    struct traceprov_aggregate_layer *current_rows_layer = get_layer(current_column_layer->rows_layer_number);;

    // This is where it gets _interesting_ (and complicated)
    // Since the records are padded, we'll fit completely in the page.
    // However, we may be reaching the end of the allocated region.
    // This is why we look at "end of memory zone" in the layer.
    // We could, totally, compute it here, but looking at cached makes things faster.
    // However, since the records are padded, the current pointer will always be EQUAL
    // to theend  pointer. That is, there cann't be a case where we'd have to check for greater or less.

    // First add the column.
    // In the case where we're using aggregation, and it is being sorted,
    // we use a more compact way for representing columns (similar to run length encoding.)
    if (TRACEPROV_SHOULD_SORT(fcinfo->context)){
        // Only need to log at the beginning.
        // and only when we're at the second group.
        // The padding is guaranteed to be 0, so this is skipped as an optimization.
        // TRACEPROV_INCREMENT_BY_PADDING(current_column_layer);
        if (is_init && main_layer->num_groups > 1){
            append_sorted_column(current_rows_layer, current_column_layer);
        }
    }else{
        // Need to log in all cases (for now.)
        grow_if_full(current_column_layer);
        *((uint64*)current_column_layer->current_row) = (agg_context->group_cnt);
        current_column_layer->current_row += sizeof(uint64);
    }

    grow_if_full(current_rows_layer);

    // This, essentially, just adds the padding to the beginning.
    // For optimization purposes, we don't actually write to this space (because it is empty)
    TRACEPROV_INCREMENT_BY_PADDING(current_rows_layer);

    int64 *pk_space = (int64*)(current_rows_layer->current_row);

    for (int pk_id = 2; pk_id < PG_NARGS(); pk_id++, pk_space++){
        *pk_space = PG_GETARG_INT64(pk_id);
    }

    current_rows_layer->current_row = (void *)pk_space;
    PG_RETURN_POINTER(agg_context);
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_finalfunc);

Datum traceprov_agg_key_finalfunc(PG_FUNCTION_ARGS){
    
    if (unlikely(PG_ARGISNULL(0))){
        PG_RETURN_NULL();
    }

    struct traceprov_agg_context *agg_context = (struct traceprov_agg_context *)PG_GETARG_POINTER(0);
    
    // Here, to re-use the code for layer setup, the group is, simply, treated as just another layer.
    // That way, we don't have recreate yet another infrastructure for groups.
 
    const int32 group_layer_number = agg_context->layer_number + 1;
    int rc = 0;

    if (unlikely(rc = initialize_local_and_layer(group_layer_number, 1, 0, false, (Node *)fcinfo->context))){
        PRINT_ON_DEBUG("Error setting up local or layer for group.");
        elog(ERROR, "Error setting up local or layer for group.");
        return 1;
    }

    struct traceprov_aggregate_layer *current_layer = get_layer(group_layer_number);
    // Here, it'll be aligned again.
    if (unlikely(current_layer->record_padding != 0)){
        elog(ERROR, "Expected padding of 0");
        PG_RETURN_NULL();
    }

    // Here, this is slightly different than the sfunc's usage of layers.
    // This is because the group number can be arbitrary (they don't need to be sequential)
    // So, we need to, unfortunately, store all the memory mappings that are created sequentially.
    // In the previous usage of layers, once we move past a page, we won't need it. Here, we can.
    
    // This is the page number that needs to be fetched (1-indexed.)
    const int32 page_number = (((agg_context->group_cnt - 1) * sizeof(int64)) / TRACEPROV_PAGE_SIZE) + 1;


    // Need to, first, setup the group-page mapping.
    if (unlikely((rc = grow_group_page_mapping(page_number, current_layer)))){
        elog(ERROR, "Error setting up space for group-page mapping");
        return rc;
    }

    // By the time we're here, the file has already been grown to handle the group.
    const int region = TRACEPROV_NUM_REGIONS_GROUP(page_number) - 1;
    void ** ptr = (void**)current_layer->current_row;

    if (region == 0){
        PG_RETURN_POINTER(((agg_context->group_cnt - 1) * sizeof(int64)) + ptr[region]);
    }
    void *new_ptr = (((agg_context->group_cnt - 1) * sizeof(int64)) + ptr[region]) - ((TRACEPROV_PAGE_SIZE)*(1 + (region - 1)*TRACEPROV_INCREMENT_GROUP_BY_PG));
    PG_RETURN_POINTER(new_ptr); 
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_combine);

Datum traceprov_agg_key_combine(PG_FUNCTION_ARGS){
    int rc = 0;

    // We cannot do anything more than this.
    // That is, we cannot try to setup main file.
    if ((rc = initialize_local_context())){
        PRINT_ON_DEBUG("Error initializing local context.");
        return rc;
    }

    struct traceprov_agg_context *reference_struct, *other;

    if (PG_ARGISNULL(0)){
        reference_struct = (struct traceprov_agg_context*)PG_GETARG_POINTER(1);
        other = NULL;
    }else if (PG_ARGISNULL(1)){
        reference_struct = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
        other = NULL;
    } else{
        reference_struct = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
        other = (struct traceprov_agg_context*)PG_GETARG_POINTER(1);
        // If ther other happened to be combined, swap.
        if (other->is_combined){
            assert(!reference_struct->is_combined);
            struct traceprov_agg_context *tmp = other;
            other = reference_struct;
            reference_struct = tmp;
        }
    }

    assert(reference_struct != NULL);
    const uint32 layer_number = reference_struct->layer_number;

    struct traceprov_aggregate_layer *main_layer = get_layer(layer_number);
    uint32 combined_layer_number = 0;
    if ((combined_layer_number = main_layer->combined_aggregate_layer_number) == 0){
        combined_layer_number = ++traceprov_current.maximum_local_layer_used;
        main_layer->combined_aggregate_layer_number = combined_layer_number;
    }
    // Here, we don't care about any layer file (it is not this function's responsibility)
    // So, we do the bare minimum, just setting up the local context (vars)
    if ((rc = initialize_local_and_layer(combined_layer_number, 1, 2, true, (Node *)fcinfo->context))){
        PRINT_ON_DEBUG("Error setting up local or layer for combine");
        return rc;
    }

    struct traceprov_aggregate_layer *current_layer = get_layer(combined_layer_number);
    main_layer->is_leader_layer = true;

    /**
     * This is slightly tricky. We need to grow in two cases
     * 1. When current row is at the end of the memory
     * 2. When current row is end of memory- 1 BUT we'd be logging two rows.
     * We handle each of them directly here.
     * Doing inference, we won't ve able to differentiate the last row. BUT, it'd always be "skipped"
     * Because the group number will be seen as zero, and won't be matched to anything.
     * We can, totally, handle this by checking it later. However, this way, we can combine 2 truncates into 1.
     * 
     * During logging the reference, there needs to be slight care, dependening on where it came from.
     * Specifically, if the reference is not combined, we only need to log it if didn't came from the main worker.
     * This is because, if it really did come from the main worker, it is already in its local file. And we need to look at the entirety of the
     * main worker anyways (because there can be groups that appear just locally, and don't exist in other workers)
     * This is mostly an optimization (we can always log the reference struct again), but we'd be logging unnecessary information in that case. 
     */

    const bool needs_logging_reference = (
        !reference_struct->is_combined
    );

    uint8 bucket = 0;

    int group_no = 0;
    const bool is_init = needs_logging_reference;
    if (!needs_logging_reference){
        group_no = reference_struct->group_cnt;
        if (TRACEPROV_SHOULD_HASH(fcinfo->context)){
            bucket = TRACEPROV_GET_BUCKET(group_no);
        }
    }else{
        // If the current worker is the remote 
        const uint64 absolute_group_number =  ++current_layer->num_groups;
        if (TRACEPROV_SHOULD_HASH(fcinfo->context)){
            bucket = (traceprov_hashint8(absolute_group_number)) % TRACEPROV_BUCKET_COUNT;
        }
        // Setting is combined here makes things simpler during inference.
        group_no = TRACEPROV_SET_IS_COMBINED(TRACEPROV_SET_BUCKET(absolute_group_number, bucket));
        reference_struct->is_combined = 1;
    }

    struct traceprov_aggregate_layer *current_column_layer = NULL;
    if (bucket == 0){
        current_column_layer = current_layer;
    }else{
        current_column_layer = get_layer(current_layer->buckets[bucket - 1]);
    }
    struct traceprov_aggregate_layer *current_rows_layer = get_layer(current_column_layer->rows_layer_number);
    if (TRACEPROV_SHOULD_SORT(fcinfo->context)){
        // In this case, we can do the same optimization done that's done for sfunc
        // when encoding group numbers.
        if (is_init && current_layer->num_groups > 1){
            append_sorted_column(current_rows_layer, current_column_layer);
        }
    }else{
        TRACEPROV_GROW_IF_TRUE(
            current_column_layer,
            ((current_column_layer->current_row == current_column_layer->end_of_memory_zone)
            || ((current_column_layer->current_row == current_column_layer->end_of_memory_zone - sizeof(uint64)) 
                && (needs_logging_reference) // need to log the reference.
                && other != NULL // Other is not null, so we'd have to log it too.
            ))
        );

        if (needs_logging_reference){
            *(uint64*)current_column_layer->current_row = group_no;
            current_column_layer->current_row += sizeof(uint64);
        }

        if (other != NULL){
            *(uint64*)current_column_layer->current_row = group_no;
            current_column_layer->current_row += sizeof(uint64);
        }
    }

    // At this point, we've written the key column.
    // Need to write the rows.
    TRACEPROV_GROW_IF_TRUE(
        current_rows_layer,
        ((current_rows_layer->current_row == current_rows_layer->end_of_memory_zone)
        || ((current_rows_layer->current_row == current_rows_layer->end_of_memory_zone - (sizeof(uint64) + sizeof(uint64))) 
            && (needs_logging_reference) // need to log the reference.
            && other != NULL // Other is not null, so we'd have to log it too.
        ))
    );

    if (needs_logging_reference){
        *(uint64 *)(current_rows_layer->current_row) = reference_struct->worker_id;
        current_rows_layer->current_row += sizeof(uint64);
        *(uint64 *)(current_rows_layer->current_row) = reference_struct->group_cnt;
        current_rows_layer->current_row += sizeof(uint64);
    }

    if (other != NULL){
        *(uint64 *)(current_rows_layer->current_row) = other->worker_id;
        current_rows_layer->current_row += sizeof(uint64);
        *(uint64 *)(current_rows_layer->current_row) = other->group_cnt;
        current_rows_layer->current_row += sizeof(uint64);
    }

    reference_struct->group_cnt = group_no;
    PG_RETURN_POINTER(reference_struct);
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_serialize);

Datum traceprov_agg_key_serialize(PG_FUNCTION_ARGS){
    struct traceprov_agg_context *context;
    StringInfoData buf;

    if (PG_ARGISNULL(0)) PG_RETURN_BYTEA_P(NULL);

    context = (struct traceprov_agg_context*) PG_GETARG_POINTER(0);

    pq_begintypsend(&buf);
    pq_sendint8(&buf, context->is_combined);
    pq_sendint64(&buf, context->group_cnt);
    pq_sendint8(&buf, context->worker_id);
    pq_sendint32(&buf, context->layer_number);

    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_deserialize);

Datum traceprov_agg_key_deserialize(PG_FUNCTION_ARGS){

    struct traceprov_agg_context *context;
    StringInfoData buf;
    MemoryContext agg_mem_context;

    if (PG_ARGISNULL(0)) PG_RETURN_POINTER(NULL);
    
    bytea *s = PG_GETARG_BYTEA_P(0);
    initStringInfo(&buf);

    buf.data = VARDATA(s);
    buf.len = VARSIZE(s) - VARHDRSZ;
    buf.cursor = 0;
    if (!AggCheckCallContext(fcinfo, &agg_mem_context))
        elog(ERROR, "aggregate function called in non-aggregate context");

    context = (struct traceprov_agg_context *) MemoryContextAlloc(agg_mem_context, sizeof(struct traceprov_agg_context));
    context->is_combined = pq_getmsgbyte(&buf);
    context->group_cnt = pq_getmsgint64(&buf);
    context->worker_id = pq_getmsgbyte(&buf);
    context->layer_number = pq_getmsgint(&buf, sizeof(int32));

    PG_RETURN_POINTER(context);
}

int grow_group_page_mapping(const int page_to_fetch, struct traceprov_aggregate_layer *layer){

    // Here, we can be a bit clever.
    // Since the contents of the group are fine being private to a worker,
    // we can simply malloc and remalloc them, rather than doing any trickery with files.
    // This significantly simplifies this, already.
    
    // If the page to fetch is 1, it'd just be 1 page.
    // For anything more than that, we'd need to see how many pages does a single increment cover.
    // For example, if TRACEPROV_INCREMENT_GROUP_BY_PG == 3, and we have page to fetch == 7,
    // the regions will be {[0, 1], [2, 5], [6, 9]}. So, the number of regions will be 3.
    const int32 num_regions = TRACEPROV_NUM_REGIONS_GROUP(page_to_fetch);

    if (layer->current_row == NULL){
        layer->current_row = malloc(sizeof(void*)*num_regions);
        memset(layer->current_row, 0, sizeof(void*)*num_regions);
    }else if (page_to_fetch > layer->size){
        layer->current_row = realloc(layer->current_row, sizeof(void*)*num_regions);
        if (DEBUG_MODE) PRINT_ON_DEBUG("clearing out from regions: %d, for: %d", TRACEPROV_NUM_REGIONS_GROUP(layer->size), (num_regions - TRACEPROV_NUM_REGIONS_GROUP(layer->size)));
        memset((void**)layer->current_row + (TRACEPROV_NUM_REGIONS_GROUP(layer->size)), 0, (num_regions - TRACEPROV_NUM_REGIONS_GROUP(layer->size)) * sizeof(void*));
    }

    if (layer->current_row == NULL){
        return 1;
    }

    void **ptr = (void **)layer->current_row;

    if (ptr[0] == NULL){
        // Set the initial mapping
        ptr[0] = layer->last_mapping;
    }

    if (page_to_fetch <= layer->size) return 0;

    // Region is 1 indexed.
    for (int region = 2; region < num_regions + 1; region++){
        // For every new region, need to grow the file.
        if (ptr[region - 1] != NULL) continue;
        const int32 new_size = 1 + (region - 1)*TRACEPROV_INCREMENT_GROUP_BY_PG;
        if (unlikely(ftruncate(layer->layer_fd, new_size * TRACEPROV_PAGE_SIZE))){
            elog(ERROR, "Error growing the layer file later");
        }
        // Need to now actually map the new portion of the file.
        void *mapped_ptr = mmap(
            NULL,
            TRACEPROV_INCREMENT_GROUP_BY_PG * TRACEPROV_PAGE_SIZE,
            PROT_WRITE,
            MAP_SHARED,
            layer->layer_fd,
            (1 + (region - 2)*TRACEPROV_INCREMENT_GROUP_BY_PG)*TRACEPROV_PAGE_SIZE
        );

        if (mapped_ptr == MAP_FAILED){
            elog(ERROR, "Error mmaping the grown group-by file");
        }
        ptr[region - 1] = mapped_ptr; 
    }
    // Basically, 1 (for the first page) + TRACEPROV_INCREMENT_GROUP_BY_PG pages for every subsequent region.
    layer->size = 1 + (num_regions - 1)*TRACEPROV_INCREMENT_GROUP_BY_PG;

    if (unlikely(layer->size < page_to_fetch)){
        elog(ERROR, "Created wrong region sizes, didn't use hint correctly.");
    }

    return 0;
}

PG_FUNCTION_INFO_V1(mark_later);

Datum mark_later(PG_FUNCTION_ARGS){

    // Can happen when, say, it was a filler column for the union.
    // Or, for example, a left join.
    if (PG_ARGISNULL(0)){
        PG_RETURN_INT64(0);
    }

    // We'll now simply set the value of this row to be 
    struct trace_file_grouped_row * row = (struct trace_file_grouped_row*)(PG_GETARG_INT64(0));
    // if (row->in_result){
    //     elog(ERROR, "Found marking an existing row!");
    // }
    row->in_result = 1;
    PG_RETURN_INT64(1);
}

PG_FUNCTION_INFO_V1(mark_later_value);

Datum mark_later_value(PG_FUNCTION_ARGS){
    if (PG_ARGISNULL(0) || PG_ARGISNULL(1)){
        PG_RETURN_INT64(0);
    }
    struct trace_file_grouped_row * row = (struct trace_file_grouped_row*)(PG_GETARG_INT64(0));
    row->in_result = PG_GETARG_INT64(1);
    PG_RETURN_INT64(PG_GETARG_INT64(1));
}

PG_FUNCTION_INFO_V1(traceprov_agg_from_ptr_sfunc);

Datum traceprov_agg_from_ptr_sfunc(PG_FUNCTION_ARGS){

    // IMPORTANT: this is the layer number of the LAST layer.
    int32 layer_number = PG_GETARG_INT32(1);
    int32 new_layer_number = PG_GETARG_INT32(2);

    MemoryContext agg_mem_context;
    if (!AggCheckCallContext(fcinfo, &agg_mem_context))
        elog(ERROR, "aggregate function called in non-aggregate context");

    const int32 group_layer_result = layer_number + 1;

    struct traceprov_aggregate_layer *current_layer = get_layer(group_layer_result);
    struct traceprov_agg_context *agg_context;
    if (PG_ARGISNULL(0)){
        agg_context = (struct traceprov_agg_context *) MemoryContextAlloc(agg_mem_context, sizeof(struct traceprov_agg_context));
        agg_context->group_cnt = ++current_layer->num_groups;
        agg_context->layer_number = new_layer_number;
    }else{
        agg_context = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
    }

    struct trace_file_grouped_row * row = (struct trace_file_grouped_row *)(PG_GETARG_INT64(3));
    row->in_result = agg_context->group_cnt;
    PG_RETURN_POINTER(agg_context);
}

PG_FUNCTION_INFO_V1(traceprov_agg_from_ptr_combine);

Datum traceprov_agg_from_ptr_combine(PG_FUNCTION_ARGS){
    elog(ERROR, "Didn't expect combine to be called");
    PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(traceprov_agg_from_ptr_serialize);

Datum traceprov_agg_from_ptr_serialize(PG_FUNCTION_ARGS){
    elog(ERROR, "Didn't expect serialize to be called");
    PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(traceprov_agg_from_ptr_deserialize);

Datum traceprov_agg_from_ptr_deserialize(PG_FUNCTION_ARGS){
    elog(ERROR, "Didn't expect deserialize to be called");
    PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(traceprov_agg_from_ptr_finalfunc);

Datum traceprov_agg_from_ptr_finalfunc(FunctionCallInfo fcinfo){
    // This works, and is fine.
    return traceprov_agg_key_finalfunc(fcinfo);
}

// These are dummy functions (mostly to benchmark the cost of calling functions from Postgres.)

PG_FUNCTION_INFO_V1(traceprov_nop_sfunc);

Datum traceprov_nop_sfunc(PG_FUNCTION_ARGS){
    PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(traceprov_nop_finalfunc);

Datum traceprov_nop_finalfunc(PG_FUNCTION_ARGS){
    PG_RETURN_INT64(1);
}


PG_FUNCTION_INFO_V1(traceprov_nop_combine);

Datum traceprov_nop_combine(PG_FUNCTION_ARGS){
    PG_RETURN_INT64(1);
}

PG_FUNCTION_INFO_V1(traceprov_nop_serialize);

Datum traceprov_nop_serialize(PG_FUNCTION_ARGS){

    StringInfoData buf;
    if (PG_ARGISNULL(0)) PG_RETURN_BYTEA_P(NULL);

    pq_begintypsend(&buf);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

PG_FUNCTION_INFO_V1(traceprov_nop_deserialize);

Datum traceprov_nop_deserialize(PG_FUNCTION_ARGS){

    PG_RETURN_POINTER(NULL);
}

uint64 perform_log(PG_FUNCTION_ARGS, bool return_pointer_version, int offset){
    int rc = 0;
    const uint32 layer_number = PG_GETARG_INT32(offset);
    // if we're in simple append mode (return_pointer_version is false), don't need to perform any marks.
    // So, in that case, ask for 1 less than pointer version, because the group number will be then filled.
    const int width = return_pointer_version ? PG_NARGS() - offset: PG_NARGS() - 1 - offset;
    if ((rc = initialize_local_and_layer(layer_number, width, 0, true, NULL))){
        PRINT_ON_DEBUG("Error setting up local or layer: %d", rc);
        elog(ERROR, "Error setting up local or layer: %d", rc);
    }

    struct traceprov_aggregate_layer *current_layer = get_layer(layer_number);

    if (unlikely(current_layer->current_row == current_layer->end_of_memory_zone)){
        if (unlikely(rc = grow_layer_file(current_layer))){
            elog(ERROR, "Received an error when growing trace file.");
        }
    }

    // Pad before.
    current_layer->current_row += current_layer->record_padding;    
    int64 *pk_space = (int64*)(current_layer->current_row);
    for (int arg_idx = 1 + offset; arg_idx < PG_NARGS(); arg_idx++, pk_space++){
        // Don't bother writing, it is 0x0 (from truncate anyways)
        if (PG_ARGISNULL(arg_idx)) continue;
        *pk_space = PG_GETARG_INT64(arg_idx);
    }
    if (return_pointer_version){
        current_layer->current_row = (void *)&pk_space[1];
        PG_RETURN_INT64(pk_space);
    }
    current_layer->current_row = pk_space;
    PG_RETURN_INT64(++current_layer->num_rows);
}

// Takes multiple input bigints, logs them, and makes a pointer out of it.
// In general, we'd want to reuse the pointers, rather than log them, and generate a pointer again.
// However, this becomes needed in a join, for example, where the pointer can be duplicated.
// In that case, the pointer is not safely unique, so need to create this structure.
PG_FUNCTION_INFO_V1(traceprov_make_ptr);

Datum traceprov_make_ptr(PG_FUNCTION_ARGS){
    PG_RETURN_POINTER(perform_log(fcinfo, true, 0));
}

PG_FUNCTION_INFO_V1(traceprov_log_entry);

Datum traceprov_log_entry(PG_FUNCTION_ARGS){
    PG_RETURN_BOOL(perform_log(fcinfo, false, 0));
}

PG_FUNCTION_INFO_V1(traceprov_log_entry_n);

Datum traceprov_log_entry_n(PG_FUNCTION_ARGS){
    const uint64 loop_count = PG_GETARG_INT64(1);
    for (uint64 counter = 0; counter < loop_count; counter++){
        perform_log(fcinfo, false, 1);
    }
    PG_RETURN_UINT64(1);
}

PG_FUNCTION_INFO_V1(traceprov_agg_key_offset_finalfunc);

Datum traceprov_agg_key_offset_finalfunc(PG_FUNCTION_ARGS){
    // We don't care about setting any layer stuff, but do want to make sure that the worker
    // is well defined.
    if (initialize_local_context()){
        elog(ERROR, "Error initializing local context!");
    }
    if (unlikely(PG_ARGISNULL(0))){
        PG_RETURN_NULL();
    }
    struct traceprov_agg_context *agg_context = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
    PG_RETURN_INT64(agg_context->group_cnt);
}
