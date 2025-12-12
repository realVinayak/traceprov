# This makes things more organized.
from traceprovpy.tools.benchmark import ExtraQuery


TRACEPROV_SYNC_TIME = lambda: ExtraQuery(
    label="traceprov_sync_time",
    query=f"$INLINE-select * from traceprov_sync_time(0);",
    runs_after_base=True,
    strict_run=True,
)

TRACEPROV_DROP_TABLE = lambda: ExtraQuery(
    label="traceprov_drop_table",
    query=f"$INLINE-DROP TABLE IF EXISTS traceprov_lineage_selectivity;",
    runs_after_base=True,
    strict_run=True,
    skip_validation=True,
)

TRACEPROV_DROP_FUNCTION = lambda: ExtraQuery(
    label="traceprov_drop_function",
    query=f"$INLINE-DROP FUNCTION IF EXISTS traceprov_infer_selectivity_bench;",
    runs_after_base=True,
    strict_run=True,
    skip_validation=True,
)

TRACEPROV_INFER_TIME = lambda: ExtraQuery(
    label="traceprov_infer_time",
    query=f"$INLINE-select * from traceprov_infer_time(1, 0, 0);",
    runs_after_base=True,
    strict_run=True,
    skip_validation=True,
)

TRACEPROV_INFER_COUNT = lambda: ExtraQuery(
    label="traceprov_infer_count",
    query=f"$INLINE-select count(*) from traceprov_lineage_selectivity;",
    runs_after_base=True,
    strict_run=True,
    skip_validation=True,
)

GPROM_LINEAGE_COUNT = lambda: ExtraQuery(
    label="count",
    query=f"$INLINE-select count(*) from gprom_lineage;",
    runs_after_materialize=True,
    strict_run=True,
    skip_validation=True,
)

TRACEPROV_DUMP_CSV = lambda: ExtraQuery(
    label="traceprov_dump_csv",
    query=f"$INLINE-select * from traceprov_layer_to_csv();",
    runs_after_base=True,
    strict_run=True,
    skip_validation=True,
)
