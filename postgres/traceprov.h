#ifndef __TRACEPROV__
#define __TRACEPROV__
#endif


#include "c.h"

#define TRACE_PROV_DIR "/var/lib/postgresql/14/main/traceprov"

#define DEFINE_TRACE_PROV_FILE(filename) TRACE_PROV_DIR ## filename

#define MAIN_TRACE_FILE             DEFINE_TRACE_PROV_FILE("/trace_file_%d.tp")
#define PARTIAL_GROUP_BY_FILE       DEFINE_TRACE_PROV_FILE("/partial_group_by_trace.tp")
#define TRACEPROV_SHARED_CONTEXT    DEFINE_TRACE_PROV_FILE("/shared_context.shm")
#define SUBQUERY_TRACE              DEFINE_TRACE_PROV_FILE("/subq_trace_%d.tp")

// 32kB is page size
#define TRACEPROV_BLOCK_SIZE 1L << 15

#define TRACEPROV_FILE_PERMISSION (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

struct trace_file_forward_row {
    int64   group_count;
};

struct trace_file_grouped_row {
    int64   in_result;
};

struct trace_file_partial_row {
    uint8   worker_id;
    int64   local_group_number;
    int64   global_group_number;
};


struct local_context {
    int32   worker_pid;
    uint8   worker_id;
    // Stores the forward row in trace file.
    struct  trace_file_forward_row *forward_row;
    // Stores the partial row (partial aggregates)
    struct  trace_file_partial_row *init_partial_row;
    struct  trace_file_partial_row *end_partial_row;

    int64   *init_subquery_row;
    int64   *end_subquery_row;

    // This is used to handle multiple agg
    int64   *group_counts;
    int32      layer_counts;
    int32      layer_counts_size;
};

struct traceprov_shared_context {
    int32   magic_word;
    uint8   main_worker_id;
    struct local_context **local_contexts;
    // Counts the number of workers.
    uint8   worker_count;
    int32   worker_context_size;
};

struct current_context {
    uint8 my_worker_id;
    int traceprov_shared_context_fd;
    struct traceprov_shared_context *shared_context;
    // This value gets cached from shared_context.
    // This is done to avoid doing the stupid array indexing on every access.
    struct local_context *local_context;
};
