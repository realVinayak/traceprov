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
extern uint32_t traceprov_thread_count;
extern bool traceprov_split_combine;
extern bool traceprov_use_compact;
extern bool traceprov_assume_null;
extern bool traceprov_force_seq_scan;
extern bool traceprov_skip_sql_cache;
extern bool traceprov_use_table_stats;
extern bool traceprov_ignore_direct_join;
#endif