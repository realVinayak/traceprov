#include "postgres.h"
#include "fmgr.h"
#include "stdio.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>

PG_MODULE_MAGIC;


// #define PROV_FILE "provfile.prov"
// #define SCRATCH_SPACE "scratch.space"

#define PROV_FILE "/var/lib/postgresql/14/main/provfile.prov"
#define SCRATCH_SPACE "/var/lib/postgresql/14/main/scratch.space"

#define DEBUG_MODE 1
#define PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
// #define PROV_FILE "provmap.map"

#define GIGA_BYTE 1024 * 1024 * 1024
#define PROV_FILE_SIZE ((long)10 * GIGA_BYTE)

typedef long int int64;
typedef int int32;

int get_error_no(){
    int err_no = errno;
    return err_no;
}

#define PRINT_ON_DEBUG(...) do {if (DEBUG_MODE) { elog(INFO, "[traceprov]: %s, %d. PID: %d\t", __FILE__, __LINE__, getpid()); elog(INFO, __VA_ARGS__); elog(INFO, "Error no: %d", get_error_no()); } } while(0)

// #define PRINT_ON_DEBUG(...) do {if (DEBUG_MODE) { printf("[traceprov]: %s, %d. PID: %d\t", __FILE__, __LINE__, getpid()); printf(__VA_ARGS__); printf("Error no: %d", get_error_no()); } } while(0)
#define SCRATCH_PAGE_SIZE 256

const int32 scratch_page_magic = 0xBADB00DE;

int map_trace_file(void *addr, void **trace_file_ptr);

struct mmap_init_row {
    int64 primary_key;
    int64 group_cnt;
};

struct mmap_later_row {
    int64 in_result;
};

// struct context {
//     struct mmap_init_row *last_fwd_ptr;
//     struct mmap_later_row *last_ptr;
//     int64 group_cnt;
// };

// These structs are local.
struct local_context {
    int worker_id;
    int64 group_count;
    // We need to this because we want to share this mapping across processes.
    void *trace_file;
    // This is put here to encapsulate the state.
    struct mmap_init_row *p_init_row;
    int should_print;
};

static struct local_context local_context_var = {
    .worker_id = -1,
    .group_count = 0,
    .trace_file = NULL,
    .p_init_row = NULL,
    .should_print = 1
};

struct scratch_space {
    int32 magic_word;
    int worker_count;
    void *trace_file;
};

inline static void print_local_context(){
    PRINT_ON_DEBUG(
        "CONTEXT->worker_count: %d,"\
        "CONTEXT->group_count: %ld,"\
        "CONTEXT->trace_file: %p," \
        "CONTEXT->p_init_row: %p" \
        "CONTEXT->should_print: %d",
        local_context_var.worker_id,
        local_context_var.group_count,
        local_context_var.trace_file,
        local_context_var.p_init_row,  
        local_context_var.should_print   
    );
}

int remove_if_exists(){
    int rc = 0;
    int can_access = access(PROV_FILE, F_OK);
    if (can_access != 0) return 0;

    if ((rc = remove(PROV_FILE)) != 0){
        PRINT_ON_DEBUG("Error removing file!\n");
        return rc;
    }else{
        PRINT_ON_DEBUG("Removed file successfully!\n");
    }
    return 0;
}

static int init_trace_file(void **trace_file_ptr){
    int rc;

    if (rc = remove_if_exists()) return rc;

    PRINT_ON_DEBUG("[traceprov]: ERR # BEFORE OPEN");

    int trace_file_fd = open(PROV_FILE, O_CREAT | O_RDWR, PERM);
    
    if (trace_file_fd < 0){
        PRINT_ON_DEBUG("Error opening file. %d\n", trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("[traceprov]: ERR # AFTER OPEN");

    PRINT_ON_DEBUG("[traceprov]: Opened initial trace file sucessfully!\n");

    off_t moved = lseek(trace_file_fd, PROV_FILE_SIZE - 1, SEEK_SET);
    if (moved == -1){
        PRINT_ON_DEBUG("[traceprov]: Error lseeking on trace file.");
        return 1;
    }

    char temp_var = 0;
    write(trace_file_fd, &temp_var, 1);

    void *ptr = mmap(
        NULL,
        PROV_FILE_SIZE,
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

    if (local_context_var.worker_id != -1) return 0;

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
            return 1;
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
        if (rc = init_trace_file(&ptr->trace_file)){
            PRINT_ON_DEBUG("Error trying to create the trace file. \n");
            goto exit_scratch_space;
        }
        local_context_var.trace_file = ptr->trace_file;
    }else{
        PRINT_ON_DEBUG("Trying to use existing trace mapping!\n");
        // We'd now need to mmap the trace file
        // But, we'll be using the static address (from the trace file)
        if (rc = map_trace_file(ptr->trace_file, &local_context_var.trace_file)){
            PRINT_ON_DEBUG("Error trying to map the trace file. \n");
            goto exit_scratch_space;
        }
    }

    // If we're here, everything went smoothly.
    // We have the lock too.
    local_context_var.worker_id = (ptr->worker_count++);
    // This way, each worker will operate in its own "zone"
    local_context_var.p_init_row = (struct mmap_init_row *)(((char*)local_context_var.trace_file) + (GIGA_BYTE * local_context_var.worker_id));

exit_scratch_space:
    // We always need to unlock the file
    if (locked){
        if(rc = flock(scratch_fd, LOCK_UN)){
            PRINT_ON_DEBUG("Error unlocking!: %d\n", rc);
        }
    }

    int previous_error = rc;
    if (scratch_fd > 0){
        // If there is an error, and the file is open, close the file.
        if (rc = close(scratch_fd)){
            PRINT_ON_DEBUG("Error closing!: %d\n", rc);
            return rc;
        }
    }
    return previous_error;

}

int map_trace_file(void *addr, void **trace_file_ptr){
    // we SHOULD NOT be creating the trace file here, again.
    int trace_file_fd = open(PROV_FILE, O_RDWR);
    if (trace_file_fd < 0){
        PRINT_ON_DEBUG("Error opening trace file: %d\n", trace_file_fd);
        return 1;
    }

    PRINT_ON_DEBUG("[traceprov]: Opened trace file later sucessfully!\n");

    void *ptr = mmap(
        addr,
        PROV_FILE_SIZE,
        PROT_WRITE,
        MAP_SHARED | MAP_FIXED,
        trace_file_fd,
        0
    );

    if (ptr == MAP_FAILED){
        PRINT_ON_DEBUG("[traceprov]: Error mmaping the file\n");

        ptr = mmap(
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
    *trace_file_ptr = ptr;
    return 0;
}

PG_FUNCTION_INFO_V1(map);


Datum map(PG_FUNCTION_ARGS){
    PRINT_ON_DEBUG("Initial error no.");
    int rc = init_local_vars();
    if (rc != 0){
        PRINT_ON_DEBUG("Error initializing the local variables!");
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

PG_FUNCTION_INFO_V1(reinit_state);

Datum reinit_state(PG_FUNCTION_ARGS){

    PRINT_ON_DEBUG("Current state.");
    print_local_context();

    local_context_var.group_count = 0;
    local_context_var.worker_id = -1;
    local_context_var.trace_file = NULL;
    local_context_var.p_init_row = NULL;
    local_context_var.should_print = 1;

    PRINT_ON_DEBUG("Reinit state.");
    print_local_context();

    PG_RETURN_INT64(0);
}



PG_FUNCTION_INFO_V1(agg_map_sfunc);
PG_FUNCTION_INFO_V1(agg_map_finalfunc);


Datum agg_map_sfunc(PG_FUNCTION_ARGS){
    int rc = 0;
    if (rc = init_local_vars()){
        PRINT_ON_DEBUG("Error initializing args: %d", rc);
        elog(ERROR, "Couldn't set up local variables!");
        return rc;
    }else{
        if (local_context_var.should_print){
            PRINT_ON_DEBUG("Initialized local args correctly!");
            print_local_context();
        }
        local_context_var.should_print = 0;
    }

    // struct context *agg_inner_context;

    // if (PG_ARGISNULL(0)){
    //     agg_inner_context = (struct context *)MemoryContextAlloc(aggcontext, sizeof(struct context));
    //     if (counter == NULL)
    //         counter = (struct mmap_init_row*)(mapped_file);
    // }else{
    //     agg_inner_context = (struct context *)PG_GETARG_POINTER(0);
    // }
    
    local_context_var.p_init_row->group_cnt = 0;
    local_context_var.p_init_row->primary_key = PG_GETARG_INT64(1);
    local_context_var.p_init_row++;
    PG_RETURN_POINTER(local_context_var.p_init_row);
}

Datum agg_map_finalfunc(PG_FUNCTION_ARGS){
    local_context_var.p_init_row->group_cnt++;
    const struct mmap_later_row * final_value = ((struct mmap_later_row*)((char*)local_context_var.trace_file + GIGA_BYTE) - local_context_var.group_count);
    PG_RETURN_INT64(final_value);
}