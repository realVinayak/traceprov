#ifndef __TRACEPROV__
#define __TRACEPROV__
#endif


#include "c.h"
#include "errno.h"
#include "utils/elog.h"

#define TRACE_PROV_DIR "/var/lib/postgresql/14/main/traceprov"

#define DEFINE_TRACE_PROV_FILE(filename) TRACE_PROV_DIR ## filename

#define MAIN_TRACE_FILE             DEFINE_TRACE_PROV_FILE("/trace_file_%d.tp")
#define PARTIAL_GROUP_BY_FILE       DEFINE_TRACE_PROV_FILE("/partial_group_by_trace.tp")
#define TRACEPROV_SHARED_CONTEXT    DEFINE_TRACE_PROV_FILE("/shared_context.shm")
#define SUBQUERY_TRACE              DEFINE_TRACE_PROV_FILE("/subq_trace_%d.tp")

// 32kB is page size
#define TRACEPROV_BLOCK_SIZE    1L << 15
#define TRACEPROV_MAX_WORKERS   256

#define TRACEPROV_FILE_PERMISSION (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define DEBUG_MODE 0

// Forward definitions.
struct trace_file_forward_row;
struct trace_file_grouped_row;
struct trace_file_partial_row;
struct local_context;
struct traceprov_aggregate_layer;
struct traceprov_shared_context;
struct current_context;

const int32 traceprov_shared_context_magic = 0xBADB00DE;

int get_error_no(){
    int err_no = errno;
    return err_no;
}

#define PRINT_ON_DEBUG(...) do { \
    if (DEBUG_MODE) { \
        elog(INFO, \
            "[traceprov]: %s, %d. PID: %d\t", \
             __FILE__, __LINE__, \
             getpid()); \ 
        elog(INFO, __VA_ARGS__); \ 
        elog(INFO, "Error no: %d", get_error_no()); \
    } } while(0) \


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

    // Stores the partial row (partial aggregates)
    struct  trace_file_partial_row *initial_partial_row;
    struct  trace_file_partial_row *current_partial_row;

    int64   *init_subquery_row;
    int64   *end_subquery_row;

    // This stores the aggregate layers.
    // Each aggregate consists of multiple mappings (see struct traceprov_aggregate_layer)
    struct  traceprov_aggregate_layer **layers;
    // The layers is resized double each time.
    uint32  layer_count;
};

// Each layer is backed by a single file.
// However, that file is grown incrementally.
// Thus, for a single file (this layer), there exist multiple non-intersecting mappings.
// Each mapping is of 32kB (or TRACEPROV_BLOCK_SIZE).
struct traceprov_aggregate_layer {
    // Defines number of PKs being logged.
    // This is NOT number of records
    int32 num_pk_records;
    // Stores the beginning of all the mappings.
    void **mapping_starts;
    // The number of mappings.
    // This, actually, gets grown via powers of two.
    // So, the mapping_starts gets resized double each time.
    // This is done to avoid resizing all the damn time
    int32 mapping_count;
    // This points to the current_row. 
    // This, will effectively lie in [mapping_starts[-1], mapping_starts[-1] + TRACEPROV_BLOCK_SIZE)
    void *current_row;
    // This stores the number of groups that this layer has seen.
    // Since this can exist in a background worker, this is always the LOCAL count of groups (and not global)
    uint32 num_groups;
};

struct traceprov_shared_context {
    int32   magic_word;
    uint8   main_worker_id;
    // Counts the number of workers.
    uint8   worker_count;
    // This stores the local contexts for all workers.
    // Given the worker id, the context can be accessed as local_contexts[worker_id]
    // It is, currently, a bit complicated to dynamically resize this.
    // So, this is statically defined to have a size of 256.
    // That is, at most, there can be 256 parallel workers.
    // That seems like a reasonable limit anyways.
    // It is complicated because ALL the workers need to see the same pointer.
    // We go to town on dynamic resizing in other cases (like aggregate layers)
    // However, dynamic sizing here would mean that we'll have to either
    //     1. Use MAP_FIXED for traceprov_shared_context (bad, and complicated)
    //     2. Map arbitrarily AND do pointer arithematic (not too bad)
    //     3. Store the local_contexts as a logical pointer, like page 0, 1. (most practical)
    struct local_context local_contexts[TRACEPROV_MAX_WORKERS];
};

struct current_context {
    uint8 my_worker_id;
    int traceprov_shared_context_fd;
    struct traceprov_shared_context *shared_context;
    // This value gets cached from shared_context.
    // This is done to avoid doing the stupid array indexing on every access.
    struct local_context *local_context;
};

#define TRACEPROV_SHARED_CONTEXT_SIZE (((sizeof(struct traceprov_shared_context) - 1) / 512) * 512)
