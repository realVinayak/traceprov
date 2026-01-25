// CPP utils
// Done this way for backwards compatibility.

#include <fcntl.h>
#include <sys/mman.h>
#include "traceprov_ext_utils.hpp"

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

    std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32 worker_count){
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
            close(fd);
        }
        return worker_local_contexts;
    }

    void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer){
        const uint64 gap = ((uint64)layer->current_row - (uint64)layer->last_mapping);
        assert(gap >= 0);
        // Now, figure out what the last mapped region will have been (or the starting address of it.)
        const uint64 infered_gap = layer->size == 1 ? 0 : (layer->size - TRACEPROV_INCREMENT_TRACE_BY_PG);
        void *final_row = (void*)((uint64)forward_row + infered_gap*TRACEPROV_PAGE_SIZE + gap);
        return final_row;
    }

}