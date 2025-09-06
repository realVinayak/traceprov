#ifndef __TRACEPROV_UTILS__
#define __TRACEPROV_UTILS__

#include "traceprov.h"

void print_layer(struct traceprov_aggregate_layer *);
int grow_layer_file(struct traceprov_aggregate_layer *current_layer);

#endif
