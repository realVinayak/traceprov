#ifndef __TRACEPROV_DERIVE__
#define __TRACEPROV_DERIVE__
#include "traceprov_node.hpp"
TraceProvDerivationSpec *get_generic_derivation_spec(
    TraceProvParseContext **p_parsed_back_context,
    TraceProvInferSetupExtra **p_extra,
    char **p_parsed_query
);
typedef struct TraceProvTableExtra {
    TraceProvPartitionInfo *partition_spec;
    TraceProvPointerContext *pointer_spec;
} TraceProvTableExtra;

typedef std::unordered_map<TraceProvLayerNumber, std::vector<std::string *> *> TraceProvLayerString;

typedef struct TraceProvInferExtra {
    TraceProvDerivationSpec *spec;
    TraceProvLayerString *layer_string;
    std::vector<duckdb_connection> *cached_connections;
} TraceProvInferExtra;

TraceProvNullMap *traceprov_infer_nulls();
TraceProvPartitionLayers *traceprov_layers_to_partition();
TraceProvStatsCollectorMap *traceprov_get_stat_columns();

#endif