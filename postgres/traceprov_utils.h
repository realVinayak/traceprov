#ifndef __TRACEPROV_UTILS__
#define __TRACEPROV_UTILS__

#include "traceprov.h"

void print_layer(struct traceprov_aggregate_layer *);
int grow_layer_file(struct traceprov_aggregate_layer *current_layer);
void *get_final_ptr(const void *forward_row, const struct traceprov_aggregate_layer *layer);

void traceprov_fail_safe_unmap(void *ptr, size_t length);

#endif
