#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64
#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64
#define __REDIRECT_NTH

#include "postgres.h"
#include "fmgr.h"
#include "stdio.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "libpq/pqformat.h"
#include "storage/procsignal.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/mman.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/shm.h>


PG_MODULE_MAGIC;


// #define PROV_FILE "provfile.prov"
// #define SCRATCH_SPACE "scratch.space"

#define PROV_FILE "/var/lib/postgresql/14/main/provfile.prov"
#define SCRATCH_SPACE "/var/lib/postgresql/14/main/scratch.space"
#define PROV_PARALLEL_TRACE "/var/lib/postgresql/14/main/provfile_partial.prov"

#define MAX_WORKERS 10

#define DEBUG_MODE 0
#define PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
// #define PROV_FILE "provmap.map"

#define GIGA_BYTE 1024 * 1024 * 1024
#define PROV_FILE_SIZE ((long)10 * GIGA_BYTE)
#define PARTITION_SIZE 128 * 1024 * 1024

typedef long int int64;
typedef int int32;

int get_error_no(){
    int err_no = errno;
    return err_no;
}

#define PRINT_ON_DEBUG(...) do {if (DEBUG_MODE) { elog(INFO, "[traceprov]: %s, %d. PID: %d\t", __FILE__, __LINE__, getpid()); elog(INFO, __VA_ARGS__); elog(INFO, "Error no: %d", get_error_no()); } } while(0)

// #define PRINT_ON_DEBUG(...) do {if (DEBUG_MODE) { printf("[traceprov]: %s, %d. PID: %d\t", __FILE__, __LINE__, getpid()); printf(__VA_ARGS__); printf("Error no: %d", get_error_no()); } } while(0)

int setup_signal_handlers();
void reinit_state_local();

enum TPROV_SIGNALS {
    REINIT  =   1,
    DUMP    =   2
};

const int32 scratch_page_magic = 0xBADB00DE;

int map_trace_file(void *addr);

struct mmap_init_row {
    int64 primary_key;
    int64 group_cnt;
};

struct mmap_later_row {
    int64 in_result;
};


struct partial_row {
    int8 worker_id;
    int64 local_group_no;
    int64 global_group_no;
};
// These structs are local.
// Each process has its copy of this.
struct local_context {
    int worker_pid;
    int worker_id;
    int64 group_count;
    // We need to this because we want to share this mapping across processes.
    void *trace_file;
    // This is put here to encapsulate the state.
    struct mmap_init_row *p_init_row;
    int should_print;
    struct partial_row *partial_row_ptr;
    void *initial_partial_row;
};


struct absolute_local_context {
    int my_worker_id;
    int scratch_fd;
    struct scratch_space *scratch_ptr;
    int64 local_group_number;
};

static struct absolute_local_context ablc = {
    .my_worker_id = -1,
    .scratch_fd = -1,
    .scratch_ptr = NULL,
    .local_group_number = 0
};

struct scratch_space {
    int32 magic_word;
    int worker_count;
    void *trace_file;
    // The count of groups seen.
    int64 group_count;
    // The number of rows written.
    int64 row_count;
    int procs[MAX_WORKERS];
    enum TPROV_SIGNALS signal;
    int expected_count;
    int8 main_worker_id;
    struct local_context locals[MAX_WORKERS];
};

int get_scratch_space(struct scratch_space **pptr, int *fd);

#define SCRATCH_PAGE_SIZE sizeof(struct scratch_space)

inline static void print_local_context(){

    if (ablc.scratch_ptr) 
        PRINT_ON_DEBUG(
            "CONTEXT->worker_count: %d, "\
            "CONTEXT->group_count: %ld, "\
            "CONTEXT->p_init_row: %p, " \
            "CONTEXT->should_print: %d, "\
            "LOCAL WORKER ID: %d, " \
            "IS BACKGROUND: %d, " \
            "PARTIAL SPACE: %p",
            ablc.scratch_ptr->locals[ablc.my_worker_id].worker_id,
            ablc.scratch_ptr->locals[ablc.my_worker_id].group_count,
            ablc.scratch_ptr->locals[ablc.my_worker_id].p_init_row,  
            ablc.scratch_ptr->locals[ablc.my_worker_id].should_print,
            ablc.my_worker_id,
            IsBackgroundWorker,
            ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr
        );
}

static int remove_if_exists(const char *file){
    int rc = 0;
    int can_access = access(file, F_OK);
    if (can_access != 0) return 0;

    if ((rc = remove(file)) != 0){
        PRINT_ON_DEBUG("Error removing file!\n");
        return rc;
    }else{
        PRINT_ON_DEBUG("Removed file successfully!\n");
    }
    return 0;
}

static int init_trace_file(void **trace_file_ptr, const char *fileName, long int size){
    int rc;

    if (rc = remove_if_exists(fileName)) return rc;

    PRINT_ON_DEBUG("[traceprov]: ERR # BEFORE OPEN");

    int trace_file_fd = open(fileName, O_CREAT | O_RDWR, PERM);
    
    if (trace_file_fd < 0){
        PRINT_ON_DEBUG("Error opening file. %d\n", trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("[traceprov]: ERR # AFTER OPEN");

    PRINT_ON_DEBUG("[traceprov]: Opened initial trace file sucessfully!\n");

    off_t moved = lseek(trace_file_fd, size - 1, SEEK_SET);
    if (moved == -1){
        PRINT_ON_DEBUG("[traceprov]: Error lseeking on trace file.");
        return 1;
    }

    char temp_var = 0;
    write(trace_file_fd, &temp_var, 1);

    void *ptr = mmap64(
        NULL,
        size,
        PROT_WRITE,
        MAP_SHARED,
        trace_file_fd,
        0
    );

    if (ptr == MAP_FAILED){
        PRINT_ON_DEBUG("[traceprov]: Error mmaping the file.\n");
        close(trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("[traceprov]: memmapped tracefile initially correctly");
    *trace_file_ptr = ptr;
    return 0;
}

// TODO:
// 1. There needs to be some mechanism for removing the scratch space. 
// Otherwise, we'd be using garbage values. Not sure what is the best way of achieving that.
static int init_local_vars(){

    if (ablc.my_worker_id != -1) return 0;

    int scratch_fd = open(SCRATCH_SPACE, O_CREAT | O_RDWR, PERM);
    int locked = 0, rc = 0;
    
    if (scratch_fd < 0){
        PRINT_ON_DEBUG("Error opening scratch file. Error code: %d\n", scratch_fd);
        rc = 1;
        goto exit_scratch_space;
    }

    PRINT_ON_DEBUG("[traceprov]: Opened scratch file succesful");

    if ((rc = flock(scratch_fd, LOCK_EX))){
        PRINT_ON_DEBUG("Error locking the file. RC: %d\n", rc);
        goto exit_scratch_space;
    }

    PRINT_ON_DEBUG("[traceprov]: Locked scratch file");
    
    locked = 1;

    int32 magic_word = 0;
    read(scratch_fd, &magic_word, sizeof(int32));

    if (magic_word != scratch_page_magic){
        // In this case, we'd be initializing the scratch file.

        off_t moved = lseek(scratch_fd, 0, SEEK_SET);
        if (moved == -1){
            PRINT_ON_DEBUG("[traceprov]: Error lseeking on scratch file.");
            rc = 1;
            goto exit_scratch_space;
        }
        
        char buff[SCRATCH_PAGE_SIZE];
        memset(buff, 0, SCRATCH_PAGE_SIZE);
        // Write the magic word.
        write(scratch_fd, &scratch_page_magic, sizeof(int32));
        write(scratch_fd, buff, SCRATCH_PAGE_SIZE - sizeof(int32));
    }

    PRINT_ON_DEBUG("[traceprov]: mmaping scratch file");

    struct scratch_space *ptr = (struct scratch_space *)(mmap(
        NULL,
        SCRATCH_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        scratch_fd,
        0
    ));

    if (ptr == MAP_FAILED){
        PRINT_ON_DEBUG("Error doing mmap for scratch space.\n");
        rc = 1;
        goto exit_scratch_space;
    }else{
        PRINT_ON_DEBUG("mmap for scratch space successful.\n");
    }

    // Check if we need to create the main trace file.
    if (ptr->trace_file == NULL){
        PRINT_ON_DEBUG("Trying to create brand new trace mapping!\n");
        // Need to create the trace file.
        if (rc = init_trace_file(&ptr->trace_file, PROV_FILE, PROV_FILE_SIZE)){
            PRINT_ON_DEBUG("Error trying to create the trace file. \n");
            goto exit_scratch_space;
        }
    }else{
        PRINT_ON_DEBUG("Trying to use existing trace mapping!\n");
        // We'd now need to mmap the trace file
        // But, we'll be using the static address (from the trace file)
        if (rc = map_trace_file(ptr->trace_file)){
            PRINT_ON_DEBUG("Error trying to map the trace file. \n");
            goto exit_scratch_space;
        }
    }

    // If we're here, everything went smoothly.
    // We have the lock too.
    ablc.my_worker_id = (ptr->worker_count++);
    if (ablc.my_worker_id >= MAX_WORKERS){
        elog(ERROR, "Maximum worker count reached!");
        rc = 1;
        goto exit_scratch_space;
    }
    // This shouldn't happen, but whatever.
    assert(MyProcPid != 0);
    // Store the myprocs, used for later sending signals.
    ptr->procs[ablc.my_worker_id] = MyProcPid;
    ptr->locals[ablc.my_worker_id].should_print = 1;

    // This way, each worker will operate in its own "zone"
    ptr->locals[ablc.my_worker_id].p_init_row = (struct mmap_init_row *)(
        ((char*)ptr->trace_file) + (PARTITION_SIZE * ablc.my_worker_id)
    );
    // This is done for sanity reasons.
    ptr->locals[ablc.my_worker_id].worker_pid =  MyProcPid;
    ptr->locals[ablc.my_worker_id].worker_id =  ablc.my_worker_id;
    
    ablc.scratch_ptr = ptr;
    ablc.scratch_fd = scratch_fd;
    // ptr->locals[ablc.my_worker_id].p_init_row->group_cnt = 1;
    // // Also add signal handler for resetting and dumping the local state.
    // if(rc = setup_signal_handlers()){
    //     PRINT_ON_DEBUG("Error setting up the signal handler");
    //     goto exit_scratch_space;
    // }else{
    //     PRINT_ON_DEBUG("Successfully added the signal handler!");
    // }

exit_scratch_space:
    // We always need to unlock the file
    int previous_error = rc;
    if (locked){
        if(rc = flock(scratch_fd, LOCK_UN)){
            PRINT_ON_DEBUG("Error unlocking!: %d\n", rc);
        }
    }

    // if (scratch_fd > 0){
    //     // If there is an error, and the file is open, close the file.
    //     if (rc = close(scratch_fd)){
    //         PRINT_ON_DEBUG("Error closing!: %d\n", rc);
    //         return rc;
    //     }
    // }
    return previous_error;

}

void hijacked_signal_handler(int signo){

    int scratch_fd = -1;
    struct scratch_space *ptr;

    if (get_scratch_space(&ptr, &scratch_fd)){
        PRINT_ON_DEBUG("Not handling signal. Calling default");
        goto default_handler;
    }

    // There can be a race condition here, so that is why the lkc.
    if ((flock(scratch_fd, LOCK_EX))){
        PRINT_ON_DEBUG("Error locking the file");
        goto default_handler;
    }

    ptr->expected_count--;

    if(flock(scratch_fd, LOCK_UN)){
        PRINT_ON_DEBUG("Error unlocking!");
    }

    if (ptr->signal == REINIT){
        PRINT_ON_DEBUG("Triggered for reinit state!");
        reinit_state_local();
        goto setup_default;
    }else if (ptr->signal == DUMP){
        PRINT_ON_DEBUG("Triggered for dump state!");
        goto setup_default;
    }

default_handler:
    if (scratch_fd > 0) close(scratch_fd);
    procsignal_sigusr1_handler(signo);
    return;

setup_default:
    if (scratch_fd > 0) close(scratch_fd);
    struct sigaction act;
    act.sa_handler = procsignal_sigusr1_handler;
    act.sa_flags = 0;

    if (sigaction(SIGUSR1, &act, NULL)){
        PRINT_ON_DEBUG("Error reinit the default");
    }
}

// This sets up the signal handlers
int setup_signal_handlers(){
    struct sigaction act;
    
    act.sa_handler = hijacked_signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGUSR1, &act, NULL)){
        PRINT_ON_DEBUG("Error getting the sighandler!");
        return 1;
    }

    return 0;

}

int map_trace_file(void *addr){
    // we SHOULD NOT be creating the trace file here, again.
    int trace_file_fd = open(PROV_FILE, O_RDWR);
    if (trace_file_fd < 0){
        PRINT_ON_DEBUG("Error opening trace file: %d\n", trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("[traceprov]: Opened trace file later sucessfully!\n");

    void *ptr = mmap64(
        addr,
        PROV_FILE_SIZE,
        PROT_WRITE,
        MAP_SHARED | MAP_FIXED,
        trace_file_fd,
        0
    );

    if (ptr == MAP_FAILED){
        PRINT_ON_DEBUG("[traceprov]: Error mmaping the file\n");

        ptr = mmap64(
            NULL,
            PROV_FILE_SIZE,
            PROT_WRITE,
            MAP_SHARED,
            trace_file_fd,
            0
        );

        if (ptr == MAP_FAILED){
            PRINT_ON_DEBUG("[traceprov]: Normal retry mmaping the file also failed!\n");
        }

        close(trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("memmapped tracefile using existing mapping successful!\n");
    return 0;
}

PG_FUNCTION_INFO_V1(map);

Datum map(PG_FUNCTION_ARGS){
    PRINT_ON_DEBUG("Initial error no.");
    int rc = init_local_vars();
    if (rc != 0){
        PRINT_ON_DEBUG("Error initializing the local variables!");
        assert(0);
        return -1;
    }
    PRINT_ON_DEBUG("Successfully initialized the local variables!");
    print_local_context();
    // struct mmap_init_row *ptr = (struct mmap_init_row *)mapped_file;
    // ptr->primary_key = PG_GETARG_INT64(0);
    // ptr->group_cnt = 0;
    // counter = ptr + 1;
    PG_RETURN_INT64(0);
}


PG_FUNCTION_INFO_V1(mark_later);

Datum mark_later(PG_FUNCTION_ARGS){

    // We'll now simply set the value of this row to be 
    struct mmap_later_row * row = (struct mmap_later_row*)(PG_GETARG_INT64(0));
    // if (row->in_result){
    //     elog(ERROR, "Found marking an existing row!");
    // }
    row->in_result = 1;
    PG_RETURN_INT64(1);
}

void reinit_state_local(){
    PRINT_ON_DEBUG("Current state.");
    print_local_context();

    ablc.my_worker_id = -1;
    ablc.scratch_fd = -1;
    ablc.scratch_ptr = NULL;
    ablc.local_group_number = 0;

    PRINT_ON_DEBUG("Reinit state.");
    print_local_context();

}

PG_FUNCTION_INFO_V1(reinit_state);

Datum reinit_state(PG_FUNCTION_ARGS){

    int scratch_fd = -1;
    struct scratch_space *ptr;
    int rc = 0;
    

    if (rc = get_scratch_space(&ptr, &scratch_fd)){
        PG_RETURN_INT64(rc);
    }

    int expected_responses = 0;
    for (int i_proc = 0; i_proc < MAX_WORKERS; i_proc++){
        if (ptr->procs[i_proc] && ptr->procs[i_proc] != MyProcPid){
            expected_responses++;
        }
    }

    ptr->signal = REINIT;
    ptr->expected_count = expected_responses;

    int blocked = 0;

    for (int i_proc = 0; i_proc < MAX_WORKERS; i_proc++){
        if (ptr->procs[i_proc]  && ptr->procs[i_proc] != MyProcPid ){
            if (kill(ptr->procs[i_proc], SIGUSR1)){
                blocked++;
                PRINT_ON_DEBUG("Error triggering on %d", ptr->procs[i_proc]);
            }
        }
    }

    while (ptr->expected_count > blocked);

    assert(blocked == expected_responses);

    // Now, doing the local reinit.
    reinit_state_local();

    remove_if_exists(PROV_FILE);
    remove_if_exists(SCRATCH_SPACE);
    remove_if_exists(PROV_PARALLEL_TRACE);
    // This needs to reinit the state across all the workers.
exit_reinit:

    if (scratch_fd > 0){
        close(scratch_fd);
    }
    PG_RETURN_INT64(rc);
}

int get_scratch_space(struct scratch_space **pptr, int *fd){
    int scratch_fd = open(SCRATCH_SPACE, O_RDWR, PERM);
    int rc = 0;
    if (scratch_fd < 0){
        PRINT_ON_DEBUG("Error opening scratch file for dump. Error code: %d\n", scratch_fd);
        return 1;
    }

    *fd = scratch_fd;

    PRINT_ON_DEBUG("[traceprov]: Opened scratch file succesful for dump");

    struct scratch_space *ptr = (struct scratch_space *)(mmap(
        NULL,
        SCRATCH_PAGE_SIZE,
        PROT_WRITE,
        MAP_SHARED,
        scratch_fd,
        0
    ));

    if (ptr == MAP_FAILED){
        PRINT_ON_DEBUG("(traceprov dump) Error doing mmap for scratch space\n");
        close(scratch_fd);
        return 1;
    }else{
        PRINT_ON_DEBUG("(traceprov dump) mmap for scratch space successful.\n");
    }

    *pptr = ptr;
    return 0;
}

PG_FUNCTION_INFO_V1(dump_state);

Datum dump_state(PG_FUNCTION_ARGS){
    // This is done to dump the current state into the scratch file.
    // The benefit is that this function can be called just once (rather than "dumping" state after each access)

    // int scratch_fd;
    // struct scratch_space *ptr;
    // int rc = 0;
    
    // if (rc = get_scratch_space(&ptr, &scratch_fd)){
    //     PG_RETURN_INT64(rc);
    // }

    // ptr->group_count = local_context_var.group_count;
    // // This is a bit complicated (since we usually use the pointers)
    // ptr->row_count = ((uint64)local_context_var.p_init_row - (uint64)local_context_var.trace_file) / sizeof(struct mmap_init_row);

    // close(scratch_fd);
    PG_RETURN_INT64(0);
}

struct traceprov_agg_context {
    int8 is_combined;
    int64 group_cnt;
    int8 worker_id;
};


// Parallel functions

PG_FUNCTION_INFO_V1(agg_map_parallel_sfunc);
PG_FUNCTION_INFO_V1(agg_map_parallel_finalfunc);
PG_FUNCTION_INFO_V1(agg_map_parallel_combine);
PG_FUNCTION_INFO_V1(agg_map_parallel_serialize);
PG_FUNCTION_INFO_V1(agg_map_parallel_deserialize);

Datum agg_map_parallel_sfunc(PG_FUNCTION_ARGS){
    int rc = 0;
    struct traceprov_agg_context *agg_inner_context;

    if (rc = init_local_vars()){
        PRINT_ON_DEBUG("Error initializing args: %d", rc);
        elog(ERROR, "Couldn't set up local variables");
        assert(0);
        return rc;
    }else{
        if (ablc.scratch_ptr->locals[ablc.my_worker_id].should_print){
            PRINT_ON_DEBUG("Initialized local args correctly!");
            print_local_context();
            ablc.scratch_ptr->locals[ablc.my_worker_id].should_print = 0;
        }   
    }

    // Currently, this will just try to count.
    if (PG_ARGISNULL(0)){
        agg_inner_context = (struct traceprov_agg_context *)malloc(sizeof(struct traceprov_agg_context));
        agg_inner_context->group_cnt = (++ablc.scratch_ptr->locals[ablc.my_worker_id].group_count);
        agg_inner_context->worker_id = ablc.my_worker_id;
        // This has not been combined.
        agg_inner_context->is_combined = 0;

    }else{
        agg_inner_context = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
    }

    struct local_context *p_local_context = &ablc.scratch_ptr->locals[ablc.my_worker_id];
    assert (p_local_context->p_init_row->group_cnt == 0);
    if (DEBUG_MODE){
        if (p_local_context->p_init_row->group_cnt != 0){
            PRINT_ON_DEBUG("Found updating a previous row.");
            elog(ERROR, "Updating previous, invalid!");
            assert(0);
        }
    }
    p_local_context->p_init_row->group_cnt = agg_inner_context->group_cnt;
    p_local_context->p_init_row->primary_key = PG_GETARG_INT64(1);
    p_local_context->p_init_row = p_local_context->p_init_row + 1;

    PG_RETURN_POINTER(agg_inner_context);
}

Datum agg_map_parallel_finalfunc(PG_FUNCTION_ARGS){

    struct traceprov_agg_context *agg_inner_context = (struct traceprov_agg_context*)PG_GETARG_POINTER(0);
    // if (!agg_inner_context->is_combined){
    //     // elog(ERROR, "Found handling an incombined state in parallel func!");
    //     assert()
    // }

    const struct mmap_later_row * final_value = (((struct mmap_later_row*)(
        (char*)ablc.scratch_ptr->trace_file + ((long)2)*GIGA_BYTE
    )) - agg_inner_context->group_cnt
    );

    PG_RETURN_INT64(final_value);
}

Datum agg_map_parallel_combine(PG_FUNCTION_ARGS){
    int rc;

    // It is possible that the main process has never seen initialized the context.
    assert(!IsBackgroundWorker);
    
    if(rc = init_local_vars()){
        PRINT_ON_DEBUG("Error initializing args: %d", rc);
        elog(ERROR, "Couldn't set up the local variables on the main process!");
        assert(0);
        return rc;
    }else{
        if (ablc.scratch_ptr->locals[ablc.my_worker_id].should_print){
            PRINT_ON_DEBUG("Initialized the local args on main process correctly!");
            print_local_context();
            ablc.scratch_ptr->locals[ablc.my_worker_id].should_print = 0;
        }
    }

    ablc.scratch_ptr->main_worker_id = ablc.my_worker_id;

    if (ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr == NULL){
        rc = init_trace_file((void**)&ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr, PROV_PARALLEL_TRACE, GIGA_BYTE);
        if (rc){
            PRINT_ON_DEBUG("Error initializing parallel trace file.%d", rc);
            elog(ERROR, "Couldn't create the parallel trace file");
            assert(0);
        }
        ablc.scratch_ptr->locals[ablc.my_worker_id].initial_partial_row = ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr;
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
        struct traceprov_agg_context *tmp;
        // If ther other happened to be 
        if (other->is_combined){
            assert(!reference_struct->is_combined);
            tmp = other;
            other = reference_struct;
            reference_struct = tmp;
        }
    }

    assert(reference_struct != NULL);

    int group_no = 0;

    struct partial_row *p_p_row = ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr;

    if (reference_struct->is_combined){
        group_no = reference_struct->group_cnt;
    }else{
        group_no = ++ablc.scratch_ptr->locals[ablc.my_worker_id].group_count;
        p_p_row->local_group_no = reference_struct->group_cnt;
        p_p_row->worker_id = reference_struct->worker_id;
        p_p_row->global_group_no = group_no;
        ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr = p_p_row + 1;
        reference_struct->is_combined = 1;
        reference_struct->group_cnt = group_no;
    }

    // We need to refetch it.
    p_p_row = ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr;

    if (other != NULL){
        p_p_row->local_group_no = other->group_cnt;
        p_p_row->worker_id = other->worker_id;
        p_p_row->global_group_no = group_no;
        ablc.scratch_ptr->locals[ablc.my_worker_id].partial_row_ptr = p_p_row + 1;
    }

    PG_RETURN_POINTER(reference_struct);
}


Datum agg_map_parallel_serialize(PG_FUNCTION_ARGS){
    struct traceprov_agg_context *context;
    StringInfoData buf;

    if (PG_ARGISNULL(0))
        PG_RETURN_BYTEA_P(NULL);

    context = (struct traceprov_agg_context *) PG_GETARG_POINTER(0);

    pq_begintypsend(&buf);
    // I mean, this should always be 0?
    pq_sendint8(&buf, context->is_combined);
    pq_sendint64(&buf, context->group_cnt);
    pq_sendint8(&buf, context->worker_id);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf)); 
}

Datum agg_map_parallel_deserialize(PG_FUNCTION_ARGS){

    struct traceprov_agg_context *context;
    StringInfoData buf;

    if (PG_ARGISNULL(0))
        PG_RETURN_POINTER(NULL);

    bytea *s = PG_GETARG_BYTEA_P(0);
    initStringInfo(&buf);

    buf.data = VARDATA(s);
    buf.len = VARSIZE(s) - VARHDRSZ;
    buf.cursor = 0;

    context = malloc(sizeof(struct traceprov_agg_context));
    context->is_combined = pq_getmsgbyte(&buf);
    context->group_cnt = pq_getmsgint64(&buf);
    context->worker_id = pq_getmsgbyte(&buf);

    PG_RETURN_POINTER(context);
}


// These are sequeuntial functions.

PG_FUNCTION_INFO_V1(agg_map_sfunc);
PG_FUNCTION_INFO_V1(agg_map_finalfunc);


Datum agg_map_sfunc(PG_FUNCTION_ARGS){

    // PRINT_ON_DEBUG("Being called with arg: %ld", PG_GETARG_INT64(1));

    int rc = 0;
    if (rc = init_local_vars()){
        PRINT_ON_DEBUG("Error initializing args: %d", rc);
        elog(ERROR, "Couldn't set up local variables!");
        assert(0);
        return rc;
    }else{
        if (ablc.scratch_ptr->locals[ablc.my_worker_id].should_print){
            PRINT_ON_DEBUG("Initialized local args correctly!");
            print_local_context();
            ablc.scratch_ptr->locals[ablc.my_worker_id].should_print = 0;
        }
    }


    struct traceprov_agg_context *agg_inner_context;

    if (PG_ARGISNULL(0)){
        agg_inner_context = (struct traceprov_agg_context *)malloc(sizeof(struct traceprov_agg_context));
        // Here, we are creating a brand new group.
        agg_inner_context->group_cnt = (++ablc.scratch_ptr->locals[ablc.my_worker_id].group_count);
        // PRINT_ON_DEBUG("Making a new context: %ld. Group count is: %ld", PG_GETARG_INT64(1), agg_inner_context->group_cnt);
    }else{
        agg_inner_context = (struct traceprov_agg_context *)PG_GETARG_POINTER(0);
        // PRINT_ON_DEBUG("Using a previous context: %ld. Group count is: %ld", PG_GETARG_INT64(1), agg_inner_context->group_cnt);
    }
    
    struct local_context *p_local_context = &ablc.scratch_ptr->locals[ablc.my_worker_id];
    p_local_context->p_init_row->group_cnt = agg_inner_context->group_cnt;
    p_local_context->p_init_row->primary_key = PG_GETARG_INT64(1);
    p_local_context->p_init_row = p_local_context->p_init_row + 1;
    PG_RETURN_POINTER(agg_inner_context);
}

Datum agg_map_finalfunc(PG_FUNCTION_ARGS){

    struct traceprov_agg_context * agg_inner_context = (struct traceprov_agg_context *)PG_GETARG_POINTER(0);

    const struct mmap_later_row * final_value = (((struct mmap_later_row*)(
        (char*)ablc.scratch_ptr->trace_file + (ablc.my_worker_id+1)*GIGA_BYTE
    )) - agg_inner_context->group_cnt
    );

    PG_RETURN_INT64(0);
}
