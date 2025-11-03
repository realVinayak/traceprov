#ifndef __TRACEPROV_PARSE_CONTEXT__
#define __TRACEPROV_PARSE_CONTEXT__
#include "c.h"

#define TRACEPROV_LAYER_INCREMENT_BOUNDARY 3
#define TRACEPROV_TICKER "/*(traceprov)*/"

typedef uint32 TraceProvLayerNumber;

typedef struct TraceProvParseContext {
    TraceProvLayerNumber global_layer_number;
    unsigned long long int unique_idx;
} TraceProvParseContext;

void tpParseInitializeContext(TraceProvParseContext *);

TraceProvLayerNumber tpParseGetLayerNumber(TraceProvParseContext *);
char *tpParseGetUniqueAlias(TraceProvParseContext *);


#endif