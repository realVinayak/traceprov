// These are portable utils.
#include "traceprov.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "file_utils.h"
#include <errno.h>

static const uint32_t traceprov_shared_context_magic = 0xBADB00DE;

void portable_elog(int level){
    if (level == INFO) return;
    if (level == ERROR)
        exit(1);
}

static inline int round_up(const int number){
    return number == 1 ? 1 : (1 << (64 - __builtin_clzl(number - 1)));
}

int initialize_file(int fd, const uint32_t *magic_word, size_t size){
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
        if(write(fd, magic_word, sizeof(uint32_t)) != sizeof(uint32_t)){
            rc = 1;
            elog(INFO, "Error writing required amount;");
            goto out;
        }
    }
out:
    return rc;
}

int fail_safe_mmap(int fd, size_t size, void **pptr){
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


int initialize_local_context(){
    
    if (traceprov_current.my_worker_id != 0) return 0;

    int rc = 0, is_locked = 0, shared_context_fd = 0, worker_layer_map_fd = 0;
    char shared_context_file_name[1024] = { 0 };
    sprintf(shared_context_file_name, TRACEPROV_SHARED_CONTEXT, DataDir);

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

    int32_t magic_word = 0;
    uint32_t maximum_layer_used = 0;

    if(read(shared_context_fd, &magic_word, sizeof(int32_t)) == -1){
        PRINT_ON_DEBUG("Had error reading in magic word.");
        goto exit_initialize_local_context;
    }

    if (magic_word != traceprov_shared_context_magic){
        // The magic word didn't match. Need to initialize the file.
        if ((rc = initialize_file(shared_context_fd, &traceprov_shared_context_magic, TRACEPROV_SHARED_CONTEXT_SIZE))){
            goto exit_initialize_local_context;
        }
        char buff[1024] = {0};
        sprintf(buff, TRACEPROV_GRAPH_FILE, DataDir);
        int graph_file_fd = open(buff, O_RDWR);
        if (graph_file_fd < 0){
            elog(INFO, "Error opening the graph file!");   
        }else{
            if((read(graph_file_fd, &maximum_layer_used, sizeof(uint32_t))) == -1){
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

    char buff_2[1024] = {0};
    sprintf(buff_2, TRACEPROV_WORKER_LAYER_MAP, DataDir, traceprov_current.my_worker_id);
    worker_layer_map_fd = open(buff_2, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);
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

int get_or_create_layer(
    uint32_t layer_number,
    struct traceprov_aggregate_layer **p_layer,
    uint32_t record_width,
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
            char layer_file_name[1024] = {0};
            sprintf(layer_file_name, TRACEPROV_PER_WORKER_FILE,  DataDir, traceprov_current.my_worker_id);
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
    char buff[1024] = {0};
    sprintf(buff, TRACEPROV_MAIN_TRACE_FILE, DataDir, layer_number, traceprov_current.my_worker_id);
    int trace_file_fd = remove_and_create(
        buff,
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
    
    const int total_record_size = ((record_width) * sizeof(uint64_t));
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

int initialize_layer_file(
    const uint32_t layer_number,
    // Specifies the length of the key of the record.
    const uint32_t key_length,
    // Specifies the length of the record, excluding keys.
    const uint32_t record_length,
    const bool set_current_row
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

    uint32_t record_layer_number = 0;
    if (record_length > 0 && layer->rows_layer_number == 0){
        record_layer_number = ++traceprov_current.maximum_local_layer_used;
        if ((rc = get_or_create_layer(record_layer_number, NULL, record_length, set_current_row, &is_already_present))){
            elog(ERROR, "Error creating the layer file (key)");
            return rc;
        }
        layer->rows_layer_number = record_layer_number;
    }

    // if (state != NULL){
    //     // If the entries in this file will be hashed, need to also make the hash buckets for them.
    //     // This effectively makes a recursive call (but the state of the next is always null)
    //     // Technically, the recursive call can be used to implement a multi-level partitioning...
    //     if (TRACEPROV_SHOULD_HASH(state) && layer->buckets[0] == 0){
    //         // Need to make the new levels
    //         // Note that we only need to construct buckets after the current layer.
    //         // because the current layer acts as the buckets for the other ones..
    //         for (int bucket_idx = 0; bucket_idx < TRACEPROV_BUCKET_COUNT - 1; bucket_idx++){
    //             const uint32_t hash_bucket_layer_number = ++traceprov_current.maximum_local_layer_used;
    //             layer->buckets[bucket_idx] = hash_bucket_layer_number;
    //             if (initialize_layer_file(hash_bucket_layer_number, key_length, record_length, true, NULL)){
    //                 elog(ERROR, "Error initializing the hash buckets");
    //             }
    //         }
    //     }
    // }
    return 0;
}

int grow_layer_file(struct traceprov_aggregate_layer *current_layer){
    int rc = 0;
    // In this case, we'd have to grow the file.
    const long int initial_size = current_layer->size;
    // Unmap previous allocation.
    if ((rc = munmap(current_layer->last_mapping, TRACEPROV_SIZE_OF_ALLOCATION(initial_size) * TRACEPROV_PAGE_SIZE))){
        elog(ERROR, "Error unmaping");
    }
    current_layer->size += TRACEPROV_INCREMENT_TRACE_BY_PG;
    const long int next_size = (current_layer->size) * TRACEPROV_PAGE_SIZE;
    if ((rc = ftruncate(current_layer->layer_fd, next_size))){
        elog(ERROR, "Error increasing the page size layer: %d", rc);
    }
    // Now, need to create the new mapping.
    void *ptr = mmap(
        NULL,
        TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        current_layer->layer_fd,
        initial_size * TRACEPROV_PAGE_SIZE
    );

    if (((ptr == MAP_FAILED))){
        elog(ERROR,
            "Error mmaping incremented trace file. %ld, %ld", 
            TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE,
            initial_size * TRACEPROV_PAGE_SIZE
        );
    }

    // Now, need to some reinitialzation.
    current_layer->end_of_memory_zone = (void*)((TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE) + (char*)ptr);
    current_layer->current_row = ptr;
    // Also set the last mapping.
    current_layer->last_mapping = ptr;
    return rc;
}

int get_error_no(){
    int err_no = errno;
    return err_no;
}