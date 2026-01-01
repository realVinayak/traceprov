#ifndef __TRACEPROV__
#define __TRACEPROV__


#include "c.h"
#include "errno.h"
#include "utils/elog.h"
#include "nodes/nodes.h"

#include <assert.h>


// Force using C's sprintf, yukkky.
// Otherwise, Postgres' sprintf will be taken.
#undef sprintf
#undef snprintf

// TODO: Make this per-process to enable concurrent traceprovs.
// The prefix here is the base dir (root of data dir.)
#define TRACE_PROV_DIR "%s/traceprov"

#define DEFINE_TRACE_PROV_FILE(filename) TRACE_PROV_DIR filename

// This will be formatted, both, the layer number and worker
#define TRACEPROV_MAIN_TRACE_FILE       DEFINE_TRACE_PROV_FILE("/trace_file_%d_%d.tp")
// This will be formatted with layer number
#define TRACEPROV_SHARED_CONTEXT        DEFINE_TRACE_PROV_FILE("/shared_context.shm")
#define TRACEPROV_PER_WORKER_FILE       DEFINE_TRACE_PROV_FILE("/worker_%d.tp")
#define TRACEPROV_GRAPH_FILE            DEFINE_TRACE_PROV_FILE("/graph.bin")
#define TRACEPROV_WORKER_LAYER_MAP      DEFINE_TRACE_PROV_FILE("/worker_%d_layers.tp")

#define TRACEPROV_NUM_REGIONS_GROUP(pgno)   (pgno == 1 ? 1 : (((pgno - 2) / TRACEPROV_INCREMENT_GROUP_BY_PG) + 2))
#define TRACEPROV_SIZE_OF_ALLOCATION(last_alloc) (last_alloc == 1 ? 1 : TRACEPROV_INCREMENT_TRACE_BY_PG)


#ifndef TRACEPROV_PAGE_SIZE_RAW
#define TRACEPROV_PAGE_SIZE_RAW 4096
#endif

// The intention here is to align with the OS' page size.
// If the OS page size is different (huge pages, or some other page size)
#ifndef TRACEPROV_PAGE_SIZE_RAW
static_assert(0, "page size not defined!");
#else
// The casting is helpful since shifts get performed using it.
#define TRACEPROV_PAGE_SIZE ((long int) TRACEPROV_PAGE_SIZE_RAW)
#endif

// Defines the maximum number of workers currently supported.
#define TRACEPROV_MAX_WORKERS           255
// Defines the maximum number of layers per worker, before it begins
// doing dynamic memory allocation.
// Essentially, if the number of layer increases more than this, it then spills
// the extra layers to a new file (instead of storing it all part of the shared context)
// This approach makes it fast for the common case where there are couple of layers
#define TRACEPROV_MAX_LAYER_PER_WORKER  32
#ifndef TRACEPROV_INCREMENT_TRACE_BY_PG
// Increase the trace file by this many number of PAGES.
#define TRACEPROV_INCREMENT_TRACE_BY_PG 4096
#endif
// Increase the group-mapping by these many pages at once.
#define TRACEPROV_INCREMENT_GROUP_BY_PG 4096

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

int get_error_no();

#define PRINT_ON_DEBUG(...) do { \
    if (DEBUG_MODE) { \
        elog(INFO,\
            "[traceprov]: %s, %d. PID: %d\t", \
             __FILE__, __LINE__,\
             getpid());\
        elog(INFO, __VA_ARGS__);\
        elog(INFO, "Error no: %d", get_error_no()); \
    } } while(0) \


struct trace_file_forward_row {
    int64   group_count;
};

static_assert(sizeof(struct trace_file_forward_row) == 8, "Invalid size");

struct trace_file_grouped_row {
    int64   in_result;
};

struct trace_file_partial_row {
    // Ugh, this is made 64 because, now, we're able to 
    // use it in layer file.
    int64   worker_id;
    int64   local_group_number;
    int64   global_group_number;
};

static_assert(sizeof(struct trace_file_partial_row) == 24, "Invalid size!");
#define TRACEPROV_PARTIAL_ROW_SIZE 32

// Bucket count (for hashing.)
#define TRACEPROV_BUCKET_COUNT 2

// Each layer is backed by a single file.
// However, that file is grown incrementally.
// Thus, for a single file (this layer), there exist multiple non-intersecting mappings.
// Each mapping is of 32kB (or TRACEPROV_BLOCK_SIZE).
struct traceprov_aggregate_layer {
    // Defines number of PKs being logged.
    // This is NOT number of records
    // This is the "width" of records logged.
    uint32 num_pk_records;
    // We only store the last mapping that it uses.
    void *last_mapping;
    // The size of the layer in pages.
    int32 size;
    // This points to the current_row. 
    // This, will effectively lie in [last_mapping, last_mapping + TRACEPROV_BLOCK_SIZE)
    void *current_row;
    // Layers can either act as base for aggregates, or simple append log, but not both.
    // Since this can exist in a background worker, this is always the LOCAL count of groups (and not global)
    union {
        // The number of groups that this layer has seen.
        uint64 num_groups;
        // The number of rows (logged)
        uint64 num_rows;
    };
    uint32 layer_number;
    // Each record gets this much padding.
    uint32 record_padding;
    // This, simply, caches the end of the current zone.
    // Otherwise, we'd waste our time computing it on every access.
    void *end_of_memory_zone;
    // Stores the fd that corresponds to this layer.
    int32 layer_fd;
    // Whether this belongs to the leader.
    // For aggregation, this is important, since combines happens on this layer.
    bool is_leader_layer;
    // If this is aggregate layer, then what strategy is used for this.
    int aggregate_strategy;
    uint32 buckets[TRACEPROV_BUCKET_COUNT - 1];
    // If it is being combined, then this stores the layer number of the combined layer.
    // This is used during inference, to correctly determine which layer file to consult.
    uint32 combined_aggregate_layer_number;
    // All layers are stored in a columnar fashion.
    // This points to the rows (because we always index the columns directly)
    uint32 rows_layer_number;
};

static_assert(sizeof(struct traceprov_aggregate_layer) < TRACEPROV_PAGE_SIZE);

#define TRACRPROV_NUM_LAYER_PER_PAGE (TRACEPROV_PAGE_SIZE / sizeof(struct traceprov_aggregate_layer))

// Local context that each worker has.
// This stores the layers.
struct local_context {
    pid_t   worker_pid;
    uint8   worker_id;
    // This stores the aggregate layers.
    // Each aggregate consists of multiple mappings (see struct traceprov_aggregate_layer)
    struct  traceprov_aggregate_layer cached_layers[TRACEPROV_MAX_LAYER_PER_WORKER];
    struct  traceprov_aggregate_layer *layers;
    // The layers is resized double each time.
    // We don't bother looking at this if we fit in cached layers,
    // So that is why this is "dynamic".
    uint32  dynamic_layer_count;
    // Stores the dynamic layers (outside of TRACEPROV_MAX_LAYER_PER_WORKER.)
    int32   layer_fd;
};

// At least the local context should be fittable in a page.
static_assert(sizeof(struct local_context) < TRACEPROV_PAGE_SIZE);

struct traceprov_shared_context {
    int32   magic_word;
    // NOTE: This exists here just for legacy.
    // TODO: Remove all the old code and get rid of this.
    // The new derivation (using graph) doesn't depend on it.
    uint8   main_worker_id;
    // Counts the number of workers.
    uint8   worker_count;
    // This gets stored in the shared context because it is otherwise hard to determine what's
    // the appropriate layer to use for combine.
    uint32  maximum_layer_number_used;
};

struct current_context {
    uint8 my_worker_id;
    int traceprov_shared_context_fd;
    struct traceprov_shared_context *shared_context;
    // This value gets cached from shared_context.
    // This is done to avoid doing the stupid array indexing on every access.
    struct local_context *local_context;
    // This is used during the logging of groups (to determine where the combiner layer goes.)
    uint32  maximum_local_layer_used;
};

struct traceprov_agg_context {
    // Whether this aggregation was combined.
    int8 is_combined;
    // Group count for this group.
    int64 group_cnt;
    // Worker on which this group was processed.
    uint8 worker_id;
    // Layer number for this group.
    uint32 layer_number;
};

uint32 traceprov_hashint8(int64);

// Whenever this condition fails, also need to update the function definition.
static_assert(sizeof(struct traceprov_agg_context) <= 32, "Expected the size of aggregate to fit in func definition size");

#define TRACEPROV_SHARED_CONTEXT_SIZE (sizeof(struct traceprov_shared_context))

#define GET_PK_FROM_ROW(PTR, PK_ID) ((int64*)(((uint8*)&(PTR->group_count)) + sizeof(PTR->group_count)) + PK_ID)

// #define TRACEPROV_SHOULD_HASH(state) (IsA(state, AggState) && ((AggState *)state)->aggstrategy == AGG_HASHED)
// #define TRACEPROV_SHOULD_SORT(state) (IsA(state, AggState) && ((AggState *)state)->aggstrategy == AGG_SORTED)

#define TRACEPROV_SHOULD_HASH(state) (false)
#define TRACEPROV_SHOULD_SORT(state) (false)

#define TRACEPROV_SET_BUCKET(X, BUCKET) ((((uint64) BUCKET) << 48) | X)
#define TRACEPROV_GET_BUCKET(X) ((uint8) (((uint64) X) >> 48))

#define TRACEPROV_SET_IS_COMBINED(X) ((((uint64)1) << 47) | X)
#define TRACEPROV_GET_IS_COMBINED(X) (((((uint64)1) << 47) & X) != 0)
#define TRACEPROV_STRIP_COMBINED(X) ((~(((uint64)1) << 47)) & X)

#define TRACEPROV_SET_WORKER_ID(X, W) ((((uint64) W) << 56) | X)
#define TRACEPROV_GET_WORKER_ID(X) ((uint8) (((uint64) X) >> 56))
#define TRACEPROV_STRIP_WORKER_ID(X) ((((uint64)(~((uint8)0))) << 56) & X)

#define TRACEPROV_INCREMENT_BY_PADDING(layer) (layer->current_row += layer->record_padding)

#define TRACEPROV_GET_RECORD_SIZE(layer) (layer->record_padding + (sizeof(uint64)*layer->num_pk_records))

#endif
