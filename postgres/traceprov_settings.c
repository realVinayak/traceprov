#include "traceprov_settings.h"

// Use prealloc?
bool traceprov_use_prealloc = true;
// Use prealloc in log?
bool traceprov_use_prealloc_log = false;
// Use prealloc in intermediate?
bool traceprov_use_prealloc_intermediate = false;
// Use compressed representation in sort.
bool traceprov_use_compressed_in_sort = false;
// Use row-id based rewrite (for duckdb)?
bool traceprov_use_rowid_duckdb = false;
// Use seq scan during log reads?
bool traceprov_force_seq_scan = false;
bool traceprov_use_table_stats = false;
char *traceprov_duckdb_profile_out = NULL;
bool traceprov_use_join_filter_rewrite = false;
bool traceprov_use_filter_pushdown = false;
bool traceprov_use_foldable = false;