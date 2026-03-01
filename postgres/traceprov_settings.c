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