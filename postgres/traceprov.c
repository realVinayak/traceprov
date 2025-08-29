#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>

#include "traceprov.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "file_utils.h"
#include "traceprov_utils.h"


PG_MODULE_MAGIC;

static const int32 traceprov_shared_context_magic = 0xBADB00DE;

static struct current_context traceprov_current = {
    .my_worker_id =                 255,
    .traceprov_shared_context_fd =  -1,
    .shared_context =               NULL,
    .local_context =                NULL
};

static int round_up(const int number){
    return number == 1 ? 1 : (1 << (64 - __builtin_clzl(number - 1)));
}

static int initialize_shared_context(int shared_context_fd){
    int rc = 0;
    // First, truncate the file to 0.
    if ((rc = ftruncate(shared_context_fd, 0))){
        PRINT_ON_DEBUG("Error truncating shared context file.");
        return rc;
    }

    off_t moved = lseek(shared_context_fd, 0, SEEK_SET);
    if (moved == -1){
        PRINT_ON_DEBUG("Error doing lseek to beginning on shared context file");
        return 1;
    }

    char buff[TRACEPROV_SHARED_CONTEXT_SIZE];
    memset(buff, 0,TRACEPROV_SHARED_CONTEXT_SIZE);
    // Write the magic word.
    write(shared_context_fd, &traceprov_shared_context_magic, sizeof(int32));
    write(shared_context_fd, buff, TRACEPROV_SHARED_CONTEXT_SIZE - sizeof(int32));
    return 0;
}

static int initialize_local_context(){
    
    if (traceprov_current.my_worker_id != 255) return 0;

    int rc = 0, is_locked = 0;

    int shared_context_fd = open(TRACEPROV_SHARED_CONTEXT, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);

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

    read(traceprov_shared_context_magic, &magic_word, sizeof(int32));

    if (magic_word != traceprov_shared_context_magic){
        // The magic word didn't match. Need to initialize the file.
        if ((rc = initialize_shared_context(shared_context_fd))){
            goto exit_initialize_local_context;
        }
    }

    PRINT_ON_DEBUG("Mmaping the shared context file.");

    struct traceprov_shared_context *shared_context = (struct traceprov_shared_context *) mmap(
        NULL,
        TRACEPROV_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        shared_context_fd,
        0
    );

    if (shared_context == MAP_FAILED){
        PRINT_ON_DEBUG("Error doing mmap for shared context.");
        rc = 1;
        goto exit_initialize_local_context;
    }else{
        PRINT_ON_DEBUG("Mmap for shared context correctly.");
    }

    // There is now an exclusive lock on the shared context file
    // Futher, it is mmaped.

    traceprov_current.my_worker_id = (shared_context->worker_count++);
    if (traceprov_current.my_worker_id >= TRACEPROV_MAX_WORKERS){
        elog(ERROR, "Maximum worker count reached.");
        rc = 1;
        goto exit_initialize_local_context;
    }

    // Set up the current context. All this is local (so, not visible to other processes.)
    traceprov_current.local_context = &shared_context->local_contexts[traceprov_current.my_worker_id];
    traceprov_current.traceprov_shared_context_fd = shared_context_fd;
    traceprov_current.shared_context = shared_context;

    // Initialize the local context (in shared)
    traceprov_current.local_context->worker_id = traceprov_current.my_worker_id;
    traceprov_current.local_context->worker_pid = MyProcPid;

    if (!IsBackgroundWorker){
        shared_context->main_worker_id = traceprov_current.my_worker_id;
    }

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
    return rc;
}

static int initialize_layer_file(const int layer_number, const int num_pk_records){

    /**
     * Here are the sequence of operations this function needs to perform.
     * 1. Make space in local_context's layers for the current layer (or reuse previous space)
     * 2. Create the trace file for this layer
     */


    int rc = 0;

    struct traceprov_aggregate_layer *layer = NULL;

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
            char *layer_file_name = get_injected_str(TRACEPROV_PER_WORKER_FILE, traceprov_current.my_worker_id, NULL);
            if (layer_file_name == NULL){
                return 1;
            }

            if (traceprov_current.local_context->dynamic_layer_count == 0){
                // Need to initialize the worker's layer file
                layer_fd = remove_and_create(layer_file_name, TRACEPROV_PAGE_SIZE);
                if (layer_fd < 0){
                    free(layer_file_name);
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
                    free(layer_file_name);
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
    if (layer->layer_number != 0) return 0;

    // Map the actual trace file for this layer.

    char *file_name = get_injected_str(TRACEPROV_MAIN_TRACE_FILE, layer_number, NULL);

    if (file_name == NULL){
        return 1;
    }

    int trace_file_fd = remove_and_create(file_name, TRACEPROV_PAGE_SIZE);
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

    layer->num_pk_records = num_pk_records;
    layer->last_mapping = trace_ptr;
    // This is skipped because it is redundantly 0, but adding it here for documentation.
    // layer->mapping_count = 0;
    layer->current_row = trace_ptr;
    // layer->num_groups = 0;
    layer->layer_number = layer_number;
    
    const int total_record_size = ((num_pk_records + 1) * sizeof(int64));
    
    layer->record_padding = round_up(total_record_size) - total_record_size;
    if (layer->record_padding < 0){
        elog(ERROR, "Found invalid padding");
        return 1;
    }

    return 0;
}

// Assumes layer has already been created.
struct traceprov_aggregate_layer *get_layer(const int layer_number){
    if (layer_number < TRACEPROV_MAX_LAYER_PER_WORKER){
        return &traceprov_current.local_context->cached_layers[layer_number - 1];
    }
    return &traceprov_current.local_context->layers[layer_number - TRACEPROV_MAX_LAYER_PER_WORKER];
}

PG_FUNCTION_INFO_V1(test_local_setup);

Datum test_local_setup(PG_FUNCTION_ARGS){
    int rc = initialize_local_context();
    if (rc) PG_RETURN_INT32(rc);
    rc = initialize_layer_file(PG_GETARG_INT32(0), PG_GETARG_INT32(1));
    print_layer(get_layer(PG_GETARG_INT32(0)));
    PG_RETURN_INT32(rc);
}

PG_FUNCTION_INFO_V1(reinit_state);

Datum reinit_state(PG_FUNCTION_ARGS){
    traceprov_current.my_worker_id = 255;
    traceprov_current.traceprov_shared_context_fd  = -1;
    traceprov_current.shared_context = NULL;
    traceprov_current.local_context = NULL;

    int rc = remove_files_from_dir(TRACE_PROV_DIR);
    PG_RETURN_INT32(rc);
}