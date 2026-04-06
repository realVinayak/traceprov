#ifndef __TP_INFER_ESSENTIALS__
#define __TP_INFER_ESSENTIALS__

#include <stdint.h>
#include <unordered_map>

extern "C" {

    // Forward declaration.
    typedef struct TraceProvBindData TraceProvBindData;
    typedef struct TraceProvInitData TraceProvInitData;

    // These are completely indepdent of postgres or duckdb (based off stdint)
    // THis is done this way so we don't have to directly map PG -> DUCKDB
    // and vice versa.
    typedef struct TraceProvRelationArgs {
        uint32_t worker_id;
        uint32_t layer_number;
    } TraceProvRelationArgs;

    typedef struct TraceProvWindowPack {
        TraceProvBindData *bind_data;
        TraceProvInitData *init_data;
    } TraceProvWindowPack;

    // Worker and Layer will fit in uint32_t. Two of them are unique enough to distinguish any combination.
    // So they are combined here togther to form a key into the context map of layers.
    // Doesn't really matter which one comes first, as long as we're consistent about it...
    #define TRACEPROV_MAKE_WORKER_LAYER_KEY(X, Y) ((uint64_t)(((uint64_t)X << 32) | (uint64_t)Y))

    typedef struct TraceProvWindowFuncExtra {
        std::unordered_map<uint64_t, TraceProvWindowPack *> *key_bind_map;
        uint64_t num_cols;
    } TraceProvWindowFuncExtra;

}
#endif