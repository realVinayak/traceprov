#include "traceprov_settings.hpp"

/** Partition options. Useful only when doing lineage derivation from an offset. */

// Use partitioning in agg?
bool traceprov_use_partition_in_agg = false;

// Use partitioning in log?
bool traceprov_use_partition_in_log = false;

// Use row format in agg partition?
bool traceprov_use_row_in_agg_partition = false;

// Skip page cache?
bool traceprov_skip_page_cache = false;

// Use implicit union? (in derivation)
bool traceprov_use_implicit_union = false;

// Use merge chunks? (in derivation)
// Kinda recommended, so maybe make this default := true????
// DuckDB isn't optimized enough to always size the chunks correctly.
// This leads to cases where in the inference where we read the chunks inefficiently.
// With this optimization, we seek ahead and merge the chunks till we fit the 2048 size.
bool traceprov_use_merge_chunks = false;

// Combine in-memory?
// warning: also affects derivation (no joins necessary to combine layer)
bool traceprov_combine_in_memory = false;

uint32_t traceprov_thread_count = 1;