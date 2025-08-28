#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>

#include "traceprov.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"



PG_MODULE_MAGIC;

static struct current_context traceprov_current = {
    .my_worker_id =                 -1,
    .traceprov_shared_context_fd =  -1,
    .shared_context =               NULL,
    .local_context =                NULL
};

int initialize_shared_context(int shared_context_fd){
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

int initialize_local_context(){
    
    if (traceprov_current.my_worker_id != -1) return 0;

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
        TRACEPROV_SHARED_CONTEXT,
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
        }
    }
    return rc;
}