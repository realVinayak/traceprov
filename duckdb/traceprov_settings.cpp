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