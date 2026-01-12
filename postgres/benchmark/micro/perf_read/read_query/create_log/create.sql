select
    traceprov_log_entry (1, :clauses)
from
    generate_series(1, :selectivity);