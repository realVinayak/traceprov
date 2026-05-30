#ifndef __TRACEPROV__
#define __TRACEPROV__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include "duckdb.hpp"
#include <unistd.h>
#include <functional>

#if TRACEPROV_SD_MODE==1

#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"

#endif

using namespace duckdb;

#define TP_STD_VECTOR_SIZE 2048

// TODO: Make this customimizable..
#define DataDir "./"
#define MyProcPid getpid()

#ifdef __APPLE__
#define MyProcTid getpid()
#else
#define MyProcTid gettid()
#endif

// TODO: Make this per-process to enable concurrent traceprovs.
// The prefix here is the base dir (root of data dir.)
#define TRACE_PROV_DIR "%s/traceprov"
#define TRACEPROV_GRAPH_FILE "/tmp/traceprov/graph.bin"

#define DEFINE_TRACE_PROV_FILE(filename) TRACE_PROV_DIR filename

// This will be formatted, both, the layer number and worker
#define TRACEPROV_MAIN_TRACE_FILE       DEFINE_TRACE_PROV_FILE("/trace_file_%d_%d.tp")
// This will be formatted with layer number
#define TRACEPROV_SHARED_CONTEXT        DEFINE_TRACE_PROV_FILE("/shared_context.shm")
#define TRACEPROV_PER_WORKER_FILE       DEFINE_TRACE_PROV_FILE("/worker_%d.tp")
// #define TRACEPROV_GRAPH_FILE            DEFINE_TRACE_PROV_FILE("/graph.bin")
#define TRACEPROV_WORKER_LAYER_MAP      DEFINE_TRACE_PROV_FILE("/worker_%d_layers.tp")

#define TRACEPROV_SIZE_OF_ALLOCATION(last_alloc) (last_alloc == 1 ? 1 : TRACEPROV_INCREMENT_TRACE_BY_PG)


#ifndef TRACEPROV_PAGE_SIZE_RAW
#define TRACEPROV_PAGE_SIZE_RAW 4096
#endif

#define TP_MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)
#define TP_MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)

// Whether to map memory page or not (otherwise file system is used)
#define TRACEPROV_USE_MMEM_PAGE 1

// Whether to use 2 MB page
#define TRACEPROV_USE_HUGE_PAGE 0

// Whether to map the memory page via huge page.
#define TRACEPROV_MAP_HUGE_PAGE 0

// if this is being changed, need to also update the query IDs used in lineage result.
#define TRACEPROV_DEBUG_PERF 0

#if TRACEPROV_USE_MMEM_PAGE==0
static_assert(TRACEPROV_MAP_HUGE_PAGE==0, "invalid config!");
#endif

// The intention here is to align with the OS' page size.
// If the OS page size is different (huge pages, or some other page size)
#ifndef TRACEPROV_PAGE_SIZE_RAW
static_assert(0, "page size not defined!");
#else
// The casting is helpful since shifts get performed using it.
#define TRACEPROV_PAGE_SIZE ((long int) TRACEPROV_PAGE_SIZE_RAW)
#endif

#if TRACEPROV_USE_HUGE_PAGE
#undef TRACEPROV_PAGE_SIZE
// 2 MB.
#define TRACEPROV_PAGE_SIZE (((long) 1024) * 1024 * 2)
// bc each page is 2MB.......
#define TRACEPROV_INCREMENT_TRACE_BY_PG 32
#endif


// This is the _backing_ page size (assumes that the OS allocates these many bytes at once)
// This is different than raw page size if we're using huge pages.
#define TRACEPROV_BACK_PAGE_SIZE TRACEPROV_PAGE_SIZE_RAW

#if TRACEPROV_MAP_HUGE_PAGE==1
// Use the huge page size as the backing page.
#undef TRACEPROV_BACK_PAGE_SIZE
#define TRACEPROV_BACK_PAGE_SIZE TRACEPROV_PAGE_SIZE
#define TRACEPROV_MMAP_FLAGS ( MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | TP_MAP_HUGE_2MB )
#else
#define TRACEPROV_MMAP_FLAGS ( MAP_PRIVATE | MAP_ANONYMOUS )
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
#define TRACEPROV_INCREMENT_TRACE_BY_PG 256
#endif

#define TRACEPROV_PG_MAPPING_INCR_STEP  4096

#define TRACEPROV_FILE_PERMISSION (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define DEBUG_MODE 0
#define VALIDATE_MODE 0
// To analyze different implementation strategies, we need to collect some statistics.
// This is usually very inefficient, so a separate compile time option for now.
#define TRACEPROV_COLLECT_STATS_MODE 0

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
            "[traceprov]: %s, %d. PID: %d\n", \
             __FILE__, __LINE__,\
             getpid());\
        elog(INFO, __VA_ARGS__);\
        elog(INFO, "Error no: %d", get_error_no()); \
    } } while(0) \

#define PRINT_ON_VALIDATE(...) do { \
    if (VALIDATE_MODE) { \
        elog(INFO, __VA_ARGS__);\
    } } while(0) \

#define DUCKDB_EXIT_ON_ERROR(state) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_MSG(state, msg) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << "error: " << msg << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_RESULT(state, result) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << duckdb_result_error(&result) << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_RUN_SHORT_QUERY(con, query, msg) { \
    duckdb_result result; \
    std::cout << "QUERY: " << query << std::endl; \
    duckdb_state state = duckdb_query(con, query, &result); \
    DUCKDB_EXIT_ON_ERROR_RESULT(state, result); \
    duckdb_destroy_result(&result); \
    std::cout << "Reached " << msg << " correctly" << std::endl; \
} \


struct trace_file_forward_row {
    uint64_t   group_count;
};

static_assert(sizeof(struct trace_file_forward_row) == 8, "Invalid size");

struct trace_file_grouped_row {
    int64_t   in_result;
};

struct trace_file_partial_row {
    // Ugh, this is made 64 because, now, we're able to 
    // use it in layer file.
    int64_t   worker_id;
    int64_t   local_group_number;
    int64_t   global_group_number;
};

static_assert(sizeof(struct trace_file_partial_row) == 24, "Invalid size!");
#define TRACEPROV_PARTIAL_ROW_SIZE 32

// Bucket count (for hashing.)
#define TRACEPROV_BUCKET_COUNT 4

typedef struct TraceProvWorkerIdx {
    uint32_t page;
    uint64_t offset;
} TraceProvWorkerIdx;

#define TRACEPROV_INLINE_STATS_SIZE 64

// To handle statistics, we try to be a bit smart.
// For cases like aggregate layer, we don't need to compute min and max values explictly
// It can be done just by looking at the group count.
// For other cases, like pointer log, we need to look explictly at whether the attribute
// can be a pointer, or can be used as a reference.
typedef struct TraceProvStatistics {
    bool is_set;
    uint64_t min_value;
    uint64_t max_value;
} TraceProvStatistics;

// Each layer is backed by a single file.
// However, that file is grown incrementally.
// Thus, for a single file (this layer), there exist multiple non-intersecting mappings.
// Each mapping is of 32kB (or TRACEPROV_BLOCK_SIZE).
struct traceprov_aggregate_layer {
    // Defines number of PKs being logged.
    // This is NOT number of records
    // This is the "width" of records logged.
    uint32_t num_pk_records;
    // We only store the last mapping that it uses.
    void *last_mapping;
    // The size of the layer in pages.
    uint32_t size;
    // This points to the current_row. 
    // This, will effectively lie in [last_mapping, last_mapping + TRACEPROV_BLOCK_SIZE)
    void *current_row;
    // Layers can either act as base for aggregates, or simple append log, but not both.
    // Since this can exist in a background worker, this is always the LOCAL count of groups (and not global)
    union {
        // The number of groups that this layer has seen.
        uint64_t num_groups;
        // The number of rows (logged)
        uint64_t num_rows;
    };
    uint32_t layer_number;
    // Each record gets this much padding.
    uint32_t record_padding;
    // This, simply, caches the end of the current zone.
    // Otherwise, we'd waste our time computing it on every access.
    void *end_of_memory_zone;
    // Stores the fd that corresponds to this layer.
    int32_t layer_fd;
    // Whether this belongs to the leader.
    // For aggregation, this is important, since combines happens on this layer.
    bool is_leader_layer;
    // If this is aggregate layer, then what strategy is used for this.
    int aggregate_strategy;
    uint32_t buckets[TRACEPROV_BUCKET_COUNT - 1];
    // If it is being combined, then this stores the layer number of the combined layer.
    // This is used during inference, to correctly determine which layer file to consult.
    uint32_t combined_aggregate_layer_number;
    // All layers are stored in a columnar fashion.
    // This points to the rows (because we always index the columns directly)
    uint32_t rows_layer_number;
    // In cases of huge mapping usage, this is grown dynamically.
    // used to emulate page table.
    void **page_mapping;
    uint32_t page_mapping_capacity;
    uint32_t page_mapping_size;
    bool read_columns_at_once;
    #if TRACEPROV_COLLECT_STATS_MODE == 1
    // If this is a combine layer, also keep track of the maximum times a group has been accummulated.
    uint64_t max_combined_times;
    #endif
    uint64_t mask;
    // This is done so that we can propagate this up later.
    // TODO: See if this slows down capture.
    uint64_t record_count;
    // If the values here can be null, it stores the layer where the nulls are stored.
    uint32_t null_layer_number;
    void *slice_vectors[TRACEPROV_BUCKET_COUNT];
    // Store stats of these many columns at once.
    TraceProvStatistics stats[TRACEPROV_INLINE_STATS_SIZE];
};


static_assert(sizeof(struct traceprov_aggregate_layer) < TRACEPROV_PAGE_SIZE);

#define TRACRPROV_NUM_LAYER_PER_PAGE (TRACEPROV_PAGE_SIZE / sizeof(struct traceprov_aggregate_layer))

// Local context that each worker has.
// This stores the layers.
struct local_context {
    pid_t   worker_pid;
    uint8_t   worker_id;
    // This stores the aggregate layers.
    // Each aggregate consists of multiple mappings (see struct traceprov_aggregate_layer)
    struct  traceprov_aggregate_layer cached_layers[TRACEPROV_MAX_LAYER_PER_WORKER];
    struct  traceprov_aggregate_layer *layers;
    // The layers is resized double each time.
    // We don't bother looking at this if we fit in cached layers,
    // So that is why this is "dynamic".
    uint32_t  dynamic_layer_count;
    // Stores the dynamic layers (outside of TRACEPROV_MAX_LAYER_PER_WORKER.)
    int32_t   layer_fd;
};

// // At least the local context should be fittable in a page.
// static_assert(sizeof(struct local_context) < TRACEPROV_PAGE_SIZE);

struct traceprov_shared_context {
    int32_t   magic_word;
    // NOTE: This exists here just for legacy.
    // TODO: Remove all the old code and get rid of this.
    // The new derivation (using graph) doesn't depend on it.
    uint8_t   main_worker_id;
    // Counts the number of workers.
    uint8_t   worker_count;
    // This gets stored in the shared context because it is otherwise hard to determine what's
    // the appropriate layer to use for combine.
    uint32_t  maximum_layer_number_used;
};

struct current_context {
    uint8_t my_worker_id;
    int traceprov_shared_context_fd;
    struct traceprov_shared_context *shared_context;
    // This value gets cached from shared_context.
    // This is done to avoid doing the stupid array indexing on every access.
    struct local_context *local_context;
    // This is used during the logging of groups (to determine where the combiner layer goes.)
    uint32_t  maximum_local_layer_used;
    uint32_t  maximum_local_layer_used_copy;
    uint64_t  local_reinit_counter;
    // 1-indexed.
    // Gets set the first time.
    // There can be some cache locality benefits, otherwise, not much,
    uint32_t page_cache_idx;
};

typedef struct AggStateExtended {
    // Usually, this will suffice.
    std::vector<uint64_t> *inline_state;
    // Occasionally, need to also store extensions.
    // This is rare, so very lazily allocated.
    std::vector<std::vector<uint64_t> *> *extended;
    // Caching this.
    uint32_t total_size;
} AggStateExtended;

struct traceprov_agg_context {
    uint64_t state;
    #if TRACEPROV_COLLECT_STATS_MODE == 1
    uint32_t combined_count;
    #endif
    AggStateExtended *extended_state;
};

// Whenever this condition fails, also need to update the function definition.
static_assert(sizeof(struct traceprov_agg_context) <= 32, "Expected the size of aggregate to fit in func definition size");

#define TRACEPROV_SHARED_CONTEXT_SIZE (sizeof(struct traceprov_shared_context))

#define GET_PK_FROM_ROW(PTR, PK_ID) ((int64_t*)(((uint8_t*)&(PTR->group_count)) + sizeof(PTR->group_count)) + PK_ID)

// #define TRACEPROV_SHOULD_HASH(state) (IsA(state, AggState) && ((AggState *)state)->aggstrategy == AGG_HASHED)
// #define TRACEPROV_SHOULD_SORT(state) (IsA(state, AggState) && ((AggState *)state)->aggstrategy == AGG_SORTED)

// To avoid calling the duckdb function at every row, we try to cache some of that.
// See, if it'd be a macro, this wouldn't be an issue. but whatever.
// If the number of cols is more than that, we still cache, just that it is malloced.
#define TRACEPROV_MAX_INLINE_CACHE_SIZE 64

#define TRACEPROV_SHOULD_HASH(state) (state->should_hash)
#define TRACEPROV_SHOULD_SORT(state) (false)

#define TRACEPROV_SET_BUCKET(X, BUCKET) ((((uint64_t) BUCKET) << 48) | X)
#define TRACEPROV_GET_BUCKET(X) ((uint8_t) (((uint64_t) X) >> 48))

#define TRACEPROV_SET_IS_COMBINED(X) ((((uint64_t)1) << 56) | X)
#define TRACEPROV_GET_IS_COMBINED(X) (((((uint64_t)1) << 56) & X) != 0)
#define TRACEPROV_STRIP_COMBINED(X) ((~(((uint64_t)1) << 56)) & X)

#define TRACEPROV_SET_WORKER_ID(X, W) ((((uint64_t) W) << 32) | X)
#define TRACEPROV_GET_WORKER_ID(X) ((uint8_t) (((uint64_t) X) >> 32))
#define TRACEPROV_STRIP_WORKER_ID(X) ((~(((uint64_t)(~((uint8_t)0))) << 32)) & X)

#define TRACEPROV_SET_LAYER(X, BUCKET) ((((uint64_t) BUCKET) << 40) | X)
#define TRACEPROV_GET_LAYER(X) ((uint8_t) (((uint64_t) X) >> 40))

#define TRACEPROV_GET_GROUP_COUNT(X) ((uint32_t)X)

// TODO: Use this everywhere.
#define INCR_BY_BYTES(X, Y) (&(((uint8_t*)X)[Y]))

#define TRACEPROV_INCREMENT_BY_PADDING(layer) (layer->current_row = INCR_BY_BYTES(layer->current_row, layer->record_padding))

#define TRACEPROV_GET_RECORD_SIZE(layer) (layer->record_padding + (sizeof(uint64_t)*layer->num_pk_records))

typedef uint32_t TraceProvLayerNumber;

typedef std::unordered_map<TraceProvLayerNumber, std::vector<uint64_t> *> TraceProvNullMap;

typedef struct TraceProvPartitionLayerItem {
    uint32_t entry_idx;
    TraceProvLayerNumber layer;
} TraceProvPartitionLayerItem;

typedef std::vector<TraceProvPartitionLayerItem> TraceProvPartitionLayers;

typedef std::unordered_map<TraceProvLayerNumber, std::vector<uint32_t> * > TraceProvStatsCollectorMap;

void traceprov_create_and_register_agg(const uint32_t max_num_args, duckdb_connection connection, TraceProvNullMap *null_map, TraceProvPartitionLayers *partition_layers, TraceProvStatsCollectorMap * stats_collector_map);

ScalarFunction *traceprov_create_reinit_state();
ScalarFunction  **traceprov_create_log_function(
    const uint32_t num_args,
    const bool is_volatile,
    TraceProvNullMap *null_map,
    const bool is_boolean
);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


typedef struct TraceProvAggExtra {
    bool ignore_gn;
    uint64_t use_part_agg;
    std::unordered_map<TraceProvLayerNumber, std::vector<uint8_t> *> *size_map;
    TraceProvNullMap *null_map;
    TraceProvPartitionLayers *partition_layers;
    TraceProvStatsCollectorMap *stats_collector_map;
    bool is_bool_return;
} TraceProvAggExtra;

// Simplifies stuff.
struct TraceProvAggBind : public FunctionData  {

    explicit TraceProvAggBind(
        TraceProvLayerNumber layer_number,
        std::vector<uint8_t> *sizes,
        bool infer_null,
        std::vector<uint64_t> *cols,
        bool should_hash
    ) :
        layer_number(layer_number),
        sizes(sizes),
        infer_null(infer_null),
        cols(cols),
        should_hash(should_hash)
        {}

    TraceProvLayerNumber layer_number;
    // Sizes of the different columns.
    // This is cached, and used just once.
    std::vector<uint8_t> *sizes;
    bool infer_null;
    std::vector<uint64_t> *cols;
    bool should_hash;
public:
    unique_ptr<FunctionData> Copy() const override {
        return make_uniq<TraceProvAggBind>(layer_number, sizes, infer_null, cols, should_hash);
    }

    bool Equals(const FunctionData &other_p) const override {
        auto &other = other_p.Cast<TraceProvAggBind>();
        return (
            layer_number == other.layer_number 
            && sizes == other.sizes 
            && infer_null == other.infer_null 
            && cols == other.cols
            && should_hash == other.should_hash
        );
    }
};

extern uint64_t traceprov_reinit_counter;

// Only usede when combining in memory.
// In the flags, also store which attributes are pointers.
// It, in all the cases, only one column is the pointer. So, this is fine.
#define TRACEPROV_SET_POINTER_COLUMN(COL, FLAG) ((((uint64_t)COL) << 8) | ((uint64_t)FLAG))
#define TRACEPROV_GET_POINTER_COLUMN(COL, FLAG) ((uint64_t)((uint8_t)(((uint64_t)FLAG) >> 8)))

typedef std::unordered_map<uint8_t, uint8_t> TraceProvWorkerPartition;

typedef struct TraceProvPartitionInfo {
    // Top level log cached data.
    // Generally safe to cache.
    std::unordered_map<TraceProvLayerNumber, void *> *cached_data;
    // This needs to be a nested data (for each layer, we can, in general, have multiple partitions for different workers.)
    // If a worker is not in the nested entry, it implies the worker doesn't have any relevant partition :)
    std::unordered_map<TraceProvLayerNumber, TraceProvWorkerPartition*> *partition_data;
    std::unordered_map<TraceProvLayerNumber, uint64_t> *layer_log_map;
    uint64_t partition_time;
} TraceProvPartitionInfo;


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) > (Y)) ? (Y) : (X))

inline void merge_stats(TraceProvStatistics *base, const TraceProvStatistics *other){
    if (!other->is_set) return;
    if (!base->is_set){
        base->min_value = other->min_value;
        base->max_value = other->max_value;
        base->is_set = true;
        return;
    }
    base->min_value = MIN(base->min_value, other->min_value);
    base->max_value = MAX(base->max_value, other->max_value);
}

extern TraceProvAggExtra *g_tp_agg_extra;

#endif
