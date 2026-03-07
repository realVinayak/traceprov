#ifndef __TRACEPROV_SETTINGS__
#define __TRACEPROV_SETTINGS__

#include <stdint.h>

extern bool traceprov_use_partition_in_agg;
extern bool traceprov_use_partition_in_log;
extern bool traceprov_use_row_in_agg_partition;
extern bool traceprov_skip_page_cache;
extern bool traceprov_use_implicit_union;
extern bool traceprov_use_merge_chunks;
extern bool traceprov_combine_in_memory;

#endif