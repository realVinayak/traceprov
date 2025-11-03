#include "traceprov_parse_context.h"
#include "postgres.h"

// to simulate classes.
TraceProvLayerNumber tpParseGetLayerNumber(TraceProvParseContext *context){
    TraceProvLayerNumber current = context->global_layer_number;
    context->global_layer_number += TRACEPROV_LAYER_INCREMENT_BOUNDARY;
    return current;
}

void tpParseInitializeContext(TraceProvParseContext *context){
    // The first layer is 1.
    context->global_layer_number = 1;
    context->unique_idx = 0;
}

char *tpParseGetUniqueAlias(TraceProvParseContext *context){
    unsigned long long int incremented = context->unique_idx++;
    return psprintf("tp_table_%lld", incremented);
}