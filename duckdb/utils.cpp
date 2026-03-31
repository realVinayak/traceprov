// These are portable utils.
#include "traceprov.hpp"
#define __GNU_SOURCE
#define __USE_MISC
#define __USE_LARGEFILE64
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "utils.hpp"
#include "file_utils.hpp"
#include <errno.h>
#include "traceprov_partition_info.hpp"
#include <mutex>
#include "utils.h"
#include "traceprov_settings.hpp"

static const uint32_t traceprov_shared_context_magic = 0xBADB00DE;

void *request_simple_page();

std::mutex simple_page_lock;

void portable_elog(int level){
    if (level == INFO) return;
    if (level == ERROR){
        //  Recursive call is safe
        elog(INFO, "Error no: %d", errno);
        elog(INFO, "Error: %s", strerror(errno));
        exit(1);
    }
}

int initialize_file(int fd, const uint32_t *magic_word, size_t size){
    int rc = 0;
    off_t moved = 0;
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

    moved = lseek(fd, 0, SEEK_SET);
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
        PROT_WRITE | PROT_READ,
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


void traceprov_write_max_used_layer(const uint32_t maximum_layer_used){
    char buff[1024] = { 0 };
    sprintf(buff, TRACEPROV_GRAPH_FILE, DataDir);
    int graph_file_fd = open(buff, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);
    if (graph_file_fd < 0){
        elog(ERROR, "Error opening the graph file for write!");
    }else{
        if((write(graph_file_fd, &maximum_layer_used, sizeof(uint32_t))) == -1){
            elog(ERROR, "Write to graph file failed!");
        }
        if(close(graph_file_fd)) elog(ERROR, "Error closing graph file");   
    }
}

std::mutex shared_context_mutex;

// This value gets adjusted without any race conditions.
// So, doesn't need any mutexes.
uint64_t traceprov_reinit_counter = 0;

void traceprov_reset_local(){

    #if TRACEPROV_USE_MMEM_PAGE

    if (traceprov_current.local_context != NULL){
        // cleanup mem stuff (unmapping)
        std::vector<void *> *free_initial_pages = new  std::vector<void *>;
        std::vector<void *> *free_later_pages = new  std::vector<void *>;
        for (uint32_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            const struct traceprov_aggregate_layer *agg_layer = &traceprov_current.local_context->cached_layers[layer_idx];
            if (agg_layer->layer_number == 0 || agg_layer->page_mapping == NULL) continue;
            for (uint32_t mapping_id = 0; mapping_id < agg_layer->page_mapping_size; mapping_id++){
                void *page_ptr = agg_layer->page_mapping[mapping_id];
                if (!traceprov_skip_page_cache){
                    if (mapping_id == 0){
                        free_initial_pages->push_back(page_ptr);
                    }else{
                        free_later_pages->push_back(page_ptr);
                    }
                }else{
                    const uint64_t page_size = mapping_id == 0 ?  TRACEPROV_PAGE_SIZE : (TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE);
                    if(munmap(page_ptr, page_size)){
                        PRINT_ON_DEBUG("Got error stage when unmapping!");
                        elog(ERROR, "Got error stage when unmapping!");
                    }
                }
            }
            free(agg_layer->page_mapping);
            if (agg_layer->layer_fd){
                close(agg_layer->layer_fd);
            }
        }
        if (!traceprov_skip_page_cache){
            TraceProvPageCacheEntry *initial_page_entry = &g_page_cache.initial_entries[TRACEPROV_PAGE_CACHE_IDX(traceprov_current)-1];
            for (uint32_t start_idx = initial_page_entry->idx; start_idx < initial_page_entry->size; start_idx++){
                // It is possible that not all the pages end up getting reused.
                // Hence, need to reclaim such pages.
                free_initial_pages->push_back(initial_page_entry->pages[start_idx]);
            }

            // Reclaim the later pages too.
            TraceProvPageCacheEntry *later_page_entry = &g_page_cache.later_entries[TRACEPROV_PAGE_CACHE_IDX(traceprov_current)-1];
            for (uint32_t start_idx = later_page_entry->idx; start_idx < later_page_entry->size; start_idx++){
                free_later_pages->push_back(later_page_entry->pages[start_idx]);
            }
            // Reset the cache entry, finally.
            initial_page_entry->idx = 0;
            initial_page_entry->size = free_initial_pages->size();
            initial_page_entry->pages = free_initial_pages->data();

            later_page_entry->idx = 0;
            later_page_entry->size = free_later_pages->size();
            later_page_entry->pages = free_later_pages->data();
        }
    }
    #endif

    traceprov_current.my_worker_id = 0;
    traceprov_current.my_worker_id = 0;
    traceprov_current.traceprov_shared_context_fd  = -1;
    traceprov_current.shared_context = NULL;
    traceprov_current.local_context = NULL;
    traceprov_current.maximum_local_layer_used = traceprov_current.maximum_local_layer_used_copy;
}

int initialize_local_context(){

    if (traceprov_current.my_worker_id != 0 && traceprov_current.local_reinit_counter == traceprov_reinit_counter) return 0;

    traceprov_reset_local();

    int rc = 0, is_locked = 0, shared_context_fd = 0, worker_layer_map_fd = 0;
    int32_t magic_word = 0;
    uint32_t maximum_layer_used = 0;
    char shared_context_file_name[1024] = { 0 };
    char buff[1024] = {0};
    char buff_2[1024] = {0};

    struct traceprov_shared_context *shared_context = NULL;
    
    shared_context_mutex.lock();

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
    // ugh.
    maximum_layer_used = traceprov_current.maximum_local_layer_used;

    if(read(shared_context_fd, &magic_word, sizeof(int32_t)) == -1){
        PRINT_ON_DEBUG("Had error reading in magic word.");
        goto exit_initialize_local_context;
    }

    if (magic_word != traceprov_shared_context_magic){
        // The magic word didn't match. Need to initialize the file.
        if ((rc = initialize_file(shared_context_fd, &traceprov_shared_context_magic, TRACEPROV_SHARED_CONTEXT_SIZE))){
            goto exit_initialize_local_context;
        }
        sprintf(buff, TRACEPROV_GRAPH_FILE, DataDir);
        int graph_file_fd = open(buff, O_RDWR);
        if (graph_file_fd < 0){
            elog(INFO, "Error opening the graph file for read!");   
        }else{
            if((read(graph_file_fd, &maximum_layer_used, sizeof(uint32_t))) == -1){
                elog(ERROR, "Read from graph file failed!");
            }
            if(close(graph_file_fd)) elog(ERROR, "Error closing graph file");   
        }
    }

    PRINT_ON_DEBUG("Mmaping the shared context file.");
    
    if ((rc = fail_safe_mmap(shared_context_fd, TRACEPROV_SHARED_CONTEXT_SIZE, (void**)&shared_context))){
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
    if ((rc = fail_safe_mmap(worker_layer_map_fd, TRACEPROV_PAGE_SIZE * (1 + ((sizeof(struct local_context) - 1) / TRACEPROV_PAGE_SIZE)), (void**)&traceprov_current.local_context))){
        goto exit_initialize_local_context;
    }
    // Set up the current context. All this is local (so, not visible to other processes.)
    traceprov_current.traceprov_shared_context_fd = shared_context_fd;
    traceprov_current.shared_context = shared_context;

    // Initialize the local context (in shared)
    traceprov_current.local_context->worker_id = traceprov_current.my_worker_id;
    traceprov_current.local_context->worker_pid = MyProcPid;
    traceprov_current.local_reinit_counter = traceprov_reinit_counter;
    
    // Don't set this in any other case.
    // This is, effectivelly, constant across the lifecycle of this thread.
    if (traceprov_current.page_cache_idx == 0)
        traceprov_current.page_cache_idx = traceprov_current.my_worker_id;;

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
        shared_context_mutex.unlock();
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

    void *trace_ptr = NULL;
    auto entry_line = &g_page_cache.initial_entries[TRACEPROV_PAGE_CACHE_IDX(traceprov_current)-1];
    int trace_file_fd = 0;
    if (entry_line->idx < entry_line->size){
        trace_ptr = entry_line->pages[entry_line->idx++];
        // elog(INFO, "rEUSING PGES!");
    }else{
        // Need to mmap the file.
        #if TRACEPROV_USE_MMEM_PAGE
        PRINT_ON_DEBUG("Mapping huge pages!");
        // Can make do with anonymous mapping.
        trace_ptr = mmap(
            NULL,
            TRACEPROV_PAGE_SIZE,
            PROT_WRITE | PROT_READ,
            TRACEPROV_MMAP_FLAGS,
            0,
            0
        );
        int trace_file_fd = 0;
        #else
        // Map the actual trace file for this layer.
        // Each worker gets its own trace file.
        char buff[1024] = {0};
        sprintf(buff, TRACEPROV_MAIN_TRACE_FILE, DataDir, layer_number, traceprov_current.my_worker_id);
        trace_file_fd = remove_and_create(
            buff,
            TRACEPROV_PAGE_SIZE
        );

        if (trace_file_fd < 0){
            return 1;
        }
        trace_ptr = mmap(
            NULL,
            TRACEPROV_PAGE_SIZE,
            PROT_WRITE,
            MAP_SHARED,
            trace_file_fd,
            0
        );
        #endif
    }

    if (trace_ptr == MAP_FAILED){
        elog(ERROR, "Mapping the trace file failed.");
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
    layer->end_of_memory_zone = TRACEPROV_PAGE_SIZE + (char *)trace_ptr;
    layer->layer_fd = trace_file_fd;
    layer->size = 1;
    if (p_layer) *p_layer = layer;

    #if TRACEPROV_USE_MMEM_PAGE
    // Need to so some huge-page specific initialization.
    void **page_mapping = (void **)malloc(sizeof(void *)*TRACEPROV_PG_MAPPING_INCR_STEP);
    memset(page_mapping, 0, sizeof(void *)*TRACEPROV_PG_MAPPING_INCR_STEP); 
    layer->page_mapping = page_mapping;
    layer->page_mapping_capacity = TRACEPROV_PG_MAPPING_INCR_STEP;
    layer->page_mapping_size = 1;
    layer->page_mapping[0] = trace_ptr;
    #endif
    return rc;
}

int initialize_layer_file(
    const uint32_t layer_number,
    // Specifies the length of the key of the record.
    const uint32_t key_length,
    // Specifies the length of the record, excluding keys.
    const uint32_t record_length,
    const bool set_current_row,
    const bool should_hash,
    const bool can_be_null
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

    if (record_length > 0 && layer->rows_layer_number == 0){
        uint32_t record_layer_number = ++traceprov_current.maximum_local_layer_used;
        if ((rc = get_or_create_layer(record_layer_number, NULL, record_length, set_current_row, &is_already_present))){
            elog(ERROR, "Error creating the layer file (key)");
            return rc;
        }
        layer->rows_layer_number = record_layer_number;
    }

    if (can_be_null && layer->null_layer_number == 0){
        uint32_t null_layer_number = ++traceprov_current.maximum_local_layer_used;
        if ((rc = get_or_create_layer(null_layer_number, NULL, record_length, set_current_row, &is_already_present))){
            elog(ERROR, "Error creating the null layer file (key)");
            return rc;
        }
        layer->null_layer_number = null_layer_number;
    }

    // If the entries in this file will be hashed, need to also make the hash buckets for them.
    // This effectively makes a recursive call (but the state of the next is always null)
    // Technically, the recursive call can be used to implement a multi-level partitioning...
    if (should_hash && layer->buckets[0] == 0){
        // Need to make the new levels
        // Note that we only need to construct buckets after the current layer.
        // because the current layer acts as the buckets for the other ones..
        for (int bucket_idx = 0; bucket_idx < TRACEPROV_BUCKET_COUNT - 1; bucket_idx++){
            const uint32_t hash_bucket_layer_number = ++traceprov_current.maximum_local_layer_used;
            layer->buckets[bucket_idx] = hash_bucket_layer_number;
            if (initialize_layer_file(hash_bucket_layer_number, key_length, record_length, true, false, can_be_null)){
                elog(ERROR, "Error initializing the hash buckets");
            }
        }
        // Here, need to also set up the slice vectors.
        // An extra lock is acquired so that we attempt to fragment only 1 page.
        // Otherwise, things will still work, but we might get slices across different pages, which is bad for performance.
        simple_page_lock.lock();
        for (int bucket_idx = 0; bucket_idx < TRACEPROV_BUCKET_COUNT; bucket_idx += 2){
            void *slice_ptr = request_simple_page();
            layer->slice_vectors[bucket_idx] = slice_ptr;
            layer->slice_vectors[bucket_idx + 1] = INCR_BY_BYTES(slice_ptr, 4096);
        }
        simple_page_lock.unlock();
    }
    return 0;
}


// variant for huge page bc other sucks
int grow_layer_file_huge(struct traceprov_aggregate_layer *current_layer){
    PRINT_ON_DEBUG("re-mapping huge pages!");
    current_layer->size += TRACEPROV_INCREMENT_TRACE_BY_PG;
    void *trace_ptr = NULL;
    auto entry_line = &g_page_cache.later_entries[TRACEPROV_PAGE_CACHE_IDX(traceprov_current)-1];
    if (entry_line->idx < entry_line->size){
        trace_ptr = entry_line->pages[entry_line->idx++];
    }else{
        trace_ptr = mmap(
            NULL,
            TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG,
            PROT_WRITE,
            TRACEPROV_MMAP_FLAGS,
            0,
            0
        );
    }
    if (trace_ptr == MAP_FAILED){
        elog(ERROR, "remap failed for huge.");
    }
    // once every 4096...
    if (unlikely(current_layer->page_mapping_size == current_layer->page_mapping_capacity)){
        current_layer->page_mapping_capacity += TRACEPROV_PG_MAPPING_INCR_STEP;
        current_layer->page_mapping = (void **)realloc(current_layer->page_mapping, sizeof(void *)*(current_layer->page_mapping_capacity));
        if (unlikely(current_layer->page_mapping == 0)){
            elog(ERROR, "failed realloc!");
        }
    }
    current_layer->page_mapping_size += 1;
    current_layer->page_mapping[current_layer->page_mapping_size - 1] = trace_ptr;
    current_layer->current_row = trace_ptr;
    // Also set the last mapping.
    current_layer->last_mapping = trace_ptr;
    current_layer->end_of_memory_zone = (void*)((TRACEPROV_INCREMENT_TRACE_BY_PG * TRACEPROV_PAGE_SIZE) + (char*)trace_ptr);

    return 0;
}

int grow_layer_file(struct traceprov_aggregate_layer *current_layer){
    if (unlikely(current_layer->layer_number == 0))
        elog(ERROR, "Expected the layer number to be filled!");
    #if TRACEPROV_USE_MMEM_PAGE
    if (current_layer->page_mapping != NULL){
        return grow_layer_file_huge(current_layer);
    }
    #else
    int rc = 0;
    // In this case, we'd have to grow the file.
    const uint64_t initial_size = current_layer->size;
    // Unmap previous allocation.
    if ((rc = munmap(current_layer->last_mapping, TRACEPROV_SIZE_OF_ALLOCATION(initial_size) * TRACEPROV_PAGE_SIZE))){
        elog(ERROR, "Error unmaping");
    }
    current_layer->size += TRACEPROV_INCREMENT_TRACE_BY_PG;
    const uint64_t next_size = (current_layer->size) * TRACEPROV_PAGE_SIZE;
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
    #endif
}

int get_error_no(){
    int err_no = errno;
    return err_no;
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

void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer){
    const uint64_t gap = ((uint64_t)layer->current_row - (uint64_t)layer->last_mapping);
    assert(gap >= 0);
    // Now, figure out what the last mapped region will have been (or the starting address of it.)
    const uint64_t infered_gap = layer->size == 1 ? 0 : (layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
    void *final_row = (void*)((uint64_t)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);
    return final_row;
}

TraceProvLayerPartition *traceprov_make_layer_partition_info(){
    auto partition_spec = new TraceProvLayerPartition;
    partition_spec->map = new std::unordered_map<TraceProvLayerNumber, TraceProvPartitionItem*>;
    return partition_spec;
}

void traceprov_add_layer_partition_info(
    TraceProvLayerPartition *partition,
    const TraceProvLayerNumber layer,
    const TraceProvLayerNumber child_layer,
    const uint64_t parition_idx,
    void *cached_value
){
    if (partition->map->find(layer) == partition->map->end()){
        TraceProvPartitionItem *item = new TraceProvPartitionItem;
        item->cached_value = cached_value;
        item->parition_idx = nullptr;
        partition->map->insert({layer, item});
    }

    if (partition->map->find(child_layer) == partition->map->end()){
        TraceProvPartitionItem *item = new TraceProvPartitionItem;
        item->cached_value = NULL;
        item->parition_idx = new std::vector<uint64_t>;
        partition->map->insert({child_layer, item});
    }
    partition->map->at(child_layer)->parition_idx->push_back(parition_idx);
}


template <typename T> std::string *serialize_int_vector(const std::vector<T> *int_vector){
    std::string vec_string = std::string();
    vec_string += "[";
    if (int_vector != nullptr){
        bool needs_sep = false;
        for (auto value: *int_vector){
            if (needs_sep)
                vec_string += ",";
            vec_string += std::to_string(value);
            needs_sep = true;
        }
    }
    vec_string += "]";
    return new std::string(vec_string);
}


std::string *serialize_parition_item(const TraceProvPartitionItem *item){
    std::string item_string = std::string();
    item_string += "{";
    item_string += "\"cached_value\": " + std::to_string((uint64_t)item->cached_value);
    item_string += ",";
    item_string += "\"partition_idx\": " + *serialize_int_vector(item->parition_idx);
    item_string += "}";
    return new std::string(item_string);
}

std::string *traceprov_serialize_partition(const TraceProvLayerPartition * partition){
    std::string new_str = std::string("");
    new_str += "{";
    bool needs_sep = false;
    for (auto pair: *partition->map){
        if (needs_sep)
            new_str += ",";
        new_str += "\"" + std::to_string(pair.first) + "\"";
        new_str += ":";
        new_str += *serialize_parition_item(pair.second);
        needs_sep = true;
    }
    new_str += "}";
    return new std::string(new_str);
}


std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32_t worker_count){
    auto worker_local_contexts = new std::vector<struct local_context *>;
    for (uint8_t worker_id = 0; worker_id < worker_count; worker_id++){
        char buff[256] = {0};
        sprintf(buff, TRACEPROV_WORKER_LAYER_MAP, DataDir, worker_id + 1);
        int fd = open(buff, O_RDONLY);
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
        close(fd);
    }
    return worker_local_contexts;
}

template <typename T> std::string construct_int_record(T value, const std::string col_name){
    return std::to_string(value) + " AS " + col_name;
}

std::string construct_string_record(const std::string value, const std::string col_name){
    return value + " AS " + col_name;
}

std::string get_layer_count(const uint64_t worker_id, const uint64_t layer_id){
    std::string cols = "0::bigint," + std::to_string(worker_id) + "::bigint," + std::to_string(layer_id) + "::bigint";
    return "(select count(*) from traceprov_read_worker_layer(" + cols + "))";
}

std::string combine_string_vector(const std::vector<std::string> vec, const std::string delim = ","){
    std::string combined = "";
    bool needs_delim = false;
    for (auto elem: vec){
        if (needs_delim)
            combined += delim;
        needs_delim = true;
        combined += elem;
    }
    return combined;
}

// Constructs a query that returns layer info.
// Done this way so that the logic to get the count can, simply, be reused, rather than recreating it.
std::string traceprov_get_layer_info_query(){
    traceprov_shared_context shared_context;
    if (map_traceprov_shared_context(&shared_context))
        elog(ERROR, "error maping shared context!");

    auto local_contexts = traceprov_get_local_contexts(shared_context.worker_count);

    std::string sql_query = "";
    std::vector<std::string> rows;
    for (auto local_context: *local_contexts){
        for (uint32_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            std::vector<std::string> record;
            const struct traceprov_aggregate_layer *layer = &local_context->cached_layers[layer_idx];
            if (layer->layer_number == 0) continue;
            auto first = construct_int_record(local_context->worker_id, "worker_id");
            record.push_back("select " + first);
            record.push_back(construct_int_record(layer->layer_number, "layer_number"));
            record.push_back(construct_int_record(layer->num_pk_records, "num_pk_records"));
            record.push_back(construct_int_record((uint64_t)layer->last_mapping, "last_mapping"));
            record.push_back(construct_int_record(layer->size, "size"));
            record.push_back(construct_int_record((uint64_t)layer->current_row, "current_row"));
            record.push_back(construct_int_record(layer->num_groups, "num_groups"));
            record.push_back(construct_int_record(layer->num_rows, "num_rows"));
            record.push_back(construct_int_record(layer->record_padding, "record_padding"));
            record.push_back(construct_int_record((uint64_t)layer->end_of_memory_zone, "end_of_memory_zone"));
            record.push_back(construct_int_record(layer->layer_fd, "layer_fd"));
            record.push_back(construct_int_record((uint64_t)layer->is_leader_layer, "is_leader_layer"));
            record.push_back(construct_int_record(layer->aggregate_strategy, "aggregate_strategy"));
            std::vector<uint32_t> hash_buckets;
            for (uint32_t idx = 0; idx < TRACEPROV_BUCKET_COUNT - 1; idx++)
                hash_buckets.push_back(layer->buckets[idx]);
            record.push_back(construct_string_record(*serialize_int_vector(&hash_buckets), "hash_buckets"));
            record.push_back(construct_int_record(layer->combined_aggregate_layer_number, "combined_aggregate_layer_number"));
            record.push_back(construct_int_record(layer->rows_layer_number, "rows_layer_number"));
            std::vector<uint64_t> page_mappings;
            for (uint32_t idx = 0; idx < layer->page_mapping_size; idx++)
                page_mappings.push_back((uint64_t)layer->page_mapping[idx]);
            record.push_back(construct_string_record(*serialize_int_vector(&page_mappings), "page_mapping"));
            record.push_back(construct_int_record(layer->page_mapping_capacity, "page_mapping_capacity"));
            record.push_back(construct_int_record(layer->page_mapping_size, "page_mapping_size"));
            if (layer->rows_layer_number){
                // Get the record count from the rows layer.
                const struct traceprov_aggregate_layer *rows_layer = &local_context->cached_layers[layer->rows_layer_number - 1];
                record.push_back(construct_int_record(rows_layer->record_count, "layer_record_count"));
            }else{
                record.push_back(construct_int_record(layer->num_rows, "layer_record_count"));
            }
            // record.push_back(
            //     construct_string_record(
            //         get_layer_count((uint64_t)local_context->worker_id, (uint64_t)layer->layer_number),
            //         "layer_record_count"
            //     )
            // );
            #if TRACEPROV_COLLECT_STATS_MODE == 1
            record.push_back(construct_int_record(layer->max_combined_times, "max_combined_times"));
            #endif
            auto combined = combine_string_vector(record);
            rows.push_back(combined);
        }   
    }
    return combine_string_vector(rows, " UNION ALL ");
}

TraceProvPageCache g_page_cache = {
    .initial_entries = NULL,
    .later_entries = NULL,
    .raw_page_cache = NULL
};

void traceprov_setup_page_cache(const uint32_t num_threads, const uint32_t page_count){
    size_t cache_line_size = sizeof(TraceProvPageCacheEntry)*num_threads;
    g_page_cache.initial_entries = (TraceProvPageCacheEntry *)malloc(cache_line_size);
    g_page_cache.later_entries = (TraceProvPageCacheEntry *)malloc(cache_line_size);
    memset(g_page_cache.initial_entries, 0, cache_line_size);
    memset(g_page_cache.later_entries, 0, cache_line_size);
    if (page_count){
       // For each thread, pre-allocate and pre-fault these many number of pages.
       for (uint32_t thread_id = 0; thread_id < num_threads; thread_id++){
            TraceProvPageCacheEntry *entry = &g_page_cache.later_entries[thread_id];
            std::vector<void *> *free_later_pages = new  std::vector<void *>;
            for (uint32_t idx = 0; idx < page_count; idx++){
                void *trace_ptr = mmap(
                    NULL,
                    TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG,
                    PROT_WRITE,
                    TRACEPROV_MMAP_FLAGS,
                    0,
                    0
                );
                if (trace_ptr == MAP_FAILED){
                    elog(ERROR, "remap failed for huge.");
                }
                // Set the first byte.
                // This is done so that the pages get pre-faulted
                ((uint8_t*)trace_ptr)[0] = 1;
                free_later_pages->push_back(trace_ptr);
            }
            entry->pages = free_later_pages->data();
            entry->size = page_count;
       }
    }
    auto raw_page_cache = new TraceProvRawPageCache;
    raw_page_cache->lock = new std::mutex;
    raw_page_cache->raw_page_entries = new std::list<TraceProvRawPageEntry *>;
    raw_page_cache->max_page_count = TRACEPROV_PAGE_SIZE / 4096;
    g_page_cache.raw_page_cache = raw_page_cache;
}

// Gets a 4k-page.
// Tries to be smart (fragments already existing page).
// Handles huge-pages, and MacOS 16K page size easily.
void *request_simple_page(){
    auto raw_page_cache = g_page_cache.raw_page_cache;
    raw_page_cache->lock->lock();
    // This is the only case where we need to add a new page.
    // Bcuz we remove the page entry entirely when all the pages from that entry have been used.
    const bool needs_new_page = raw_page_cache->raw_page_entries->size() == 0;
    if (needs_new_page){
        void *trace_ptr = mmap(
            NULL,
            TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG,
            PROT_WRITE,
            TRACEPROV_MMAP_FLAGS,
            0,
            0
        );
        if (trace_ptr == MAP_FAILED){
            elog(ERROR, "remap failed for huge.");
        }
        TraceProvRawPageEntry *raw_page_entry = new TraceProvRawPageEntry;
        raw_page_entry->page = trace_ptr;
        raw_page_entry->page_used = 0;
        raw_page_cache->raw_page_entries->push_back(raw_page_entry);
    }
    auto raw_page_entry = raw_page_cache->raw_page_entries->front();
    void *return_ptr = INCR_BY_BYTES(raw_page_entry->page, (4096)*(raw_page_entry->page_used++));
    if (raw_page_entry->page_used == raw_page_cache->max_page_count){
        raw_page_cache->raw_page_entries->pop_front();
    }
    raw_page_cache->lock->unlock();
    return return_ptr;
}