#include <unistd.h>
#include <sys/file.h>

#include "traceprov.h"
#include "postgres.h"
#include "fmgr.h"



PG_MODULE_MAGIC;

static struct current_context current = {
    .my_worker_id =                 -1,
    .traceprov_shared_context_fd =  -1,
    .shared_context =               NULL,
    .local_context =                NULL
};

int initialize_local_context(){
    
    if (current.my_worker_id != -1) return 0;

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
        // The magic word didn't match.
        // First, truncate the file to 0.
        if ((rc = ftruncate(shared_context_fd, 0))){
            PRINT_ON_DEBUG("Error truncating shared context file.");
            goto exit_initialize_local_context;
        }
    }

exit_initialize_local_context:
    return rc;
}