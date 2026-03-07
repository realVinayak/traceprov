#ifndef __TRACEPROV_DERIVE__
#define __TRACEPROV_DERIVE__
#include "traceprov_node.hpp"
TraceProvDerivationSpec *get_generic_derivation_spec(
    TraceProvParseContext **p_parsed_back_context,
    TraceProvInferSetupExtra **p_extra,
    char **p_parsed_query
);
typedef struct TraceProvTableExtra {
    void *partition_spec;
    void *pointer_spec;
} TraceProvTableExtra;
#endif