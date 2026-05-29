#ifndef __TRACEPROV_SETTINGS__
#define __TRACEPROV_SETTINGS__

#include "c.h"

extern bool traceprov_use_prealloc;
extern bool traceprov_use_prealloc_log;
extern bool traceprov_use_prealloc_intermediate;
extern bool traceprov_use_compressed_in_sort;
extern bool traceprov_use_rowid_duckdb;
extern bool traceprov_force_seq_scan;
extern bool traceprov_use_table_stats;
extern char *traceprov_duckdb_profile_out;
#endif