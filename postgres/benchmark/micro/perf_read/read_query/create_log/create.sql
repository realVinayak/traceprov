select
    traceprov_log_entry (1, 0, :clauses)
from
    generate_series(1, :selectivity);