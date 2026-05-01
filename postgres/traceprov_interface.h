#ifndef __TRACEPROV_INTERFACE__
#define __TRACEPROV_INTERFACE__

#include "postgres.h"
#include "nodes/pg_list.h"

List *traceprov_get_null_columns(const uint32_t query_layer);
void traceprov_reset_interface();

#endif