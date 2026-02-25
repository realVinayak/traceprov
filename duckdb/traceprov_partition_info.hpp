#ifndef __TRACEPROV_PARTITION_INFO__
#include "traceprov.hpp"
#include <vector>
#include <unordered_map>

typedef struct TraceProvPartitionItem {
    void *cached_value;
    std::vector<uint64_t>* parition_idx;
} TraceProvPartitionItem;

typedef struct TraceProvLayerPartition {
    std::unordered_map<TraceProvLayerNumber, TraceProvPartitionItem*> *map;
} TraceProvLayerPartition;

TraceProvLayerPartition *traceprov_make_layer_partition_info();

void traceprov_add_layer_partition_info(
    TraceProvLayerPartition *partition,
    const TraceProvLayerNumber layer,
    const TraceProvLayerNumber child_layer,
    // This gets set in the child.
    const uint64_t parition_idx,
    // This gets set in the cached value.
    void *cached_value
);

#endif