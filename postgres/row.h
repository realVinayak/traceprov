#ifndef __TRACEPROV_ROW__

#define __TRACEPROV_ROW__
#include "c.h"

#define PROV_FILE "/var/lib/postgresql/14/main/provfile.prov"
#define SCRATCH_SPACE "/var/lib/postgresql/14/main/scratch.space"
#define PROV_PARALLEL_TRACE "/var/lib/postgresql/14/main/provfile_partial.prov"
#define PROV_SUBQ_TRACE "/var/lib/postgresql/14/main/prov_subq_trace_%d.prov"
#define PROV_SUB_FILE "/var/lib/postgresql/14/main/provfile_%d.prov"


#define GIGA_BYTE 1024 * 1024 * 1024
#define PROV_FILE_SIZE ((long)10 * GIGA_BYTE)
#define PARTITION_SIZE 128 * 1024 * 1024
#define PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define SUB_PROV_FILE_SIZE (long)2*GIGA_BYTE

#define MAX_WORKERS 10

#define SCRATCH_PAGE_SIZE sizeof(struct scratch_space)

enum TPROV_SIGNALS {
    REINIT  =   1,
    DUMP    =   2
};

struct mmap_init_row {
    // This is nice because, now, we can store multiple
    // pks in just one row. That way, we can handle joins
    // much more easily.
    int32 num_records;
    int64 group_cnt;
    // int64 *primary_keys;
};

#define MMAP_INIT_ROW_PK(PTR, PK_ID) ((int64*)(&(PTR->group_cnt) + sizeof(PTR->group_cnt)) + PK_ID)

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
    // This is put here to encapsulate the state.
    struct mmap_init_row *p_init_row;
    int should_print;
    struct partial_row *partial_row_ptr;
    void *initial_partial_row;
    int64 *subq_pk;
    int64 *initial_subq_pk;
    void *layer_mark;
    int64 second_group_count;
    void *local_trace_file;
};

struct absolute_local_context {
    int my_worker_id;
    int scratch_fd;
    struct scratch_space *scratch_ptr;
    int64 local_group_number;
    void **background_ptrs;
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
    int64 *subq_pk;
    struct local_context locals[MAX_WORKERS];
};


#endif


