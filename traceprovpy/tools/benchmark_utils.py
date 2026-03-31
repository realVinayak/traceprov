# This makes things more organized.
from datetime import datetime
import json
from pathlib import Path
from typing import Any
from traceprovpy.tools.benchmark import ExtraQuery
from traceprovpy.tools.callable_repr import CallableRepr
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, MakeTraceProv
import os

from traceprovpy.tools.traceprov_extra_func import get_traceprov_extra_infer_func

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

TRACEPROV_CAPTURE_QUERY = lambda: ExtraQuery(
    label="traceprov_capture_query",
    query=f"$INLINE-select * from traceprov_parsed_back();",
    runs_after_base=True,
    strict_run=True,
    capture_output=True,
)


TRACEPROV_PERFORM_DERIVATION = lambda: ExtraQuery(
    label="traceprov_capture_query",
    query=f"$INLINE-select * from traceprov_dump_derivation();",
    runs_after_base=True,
    strict_run=True,
    capture_output=True,
)

TRACEPROV_SQL_DERIVATION_QUERY = "select * from traceprov_get_sql_derivation();"


TRACEPROV_GET_DERIVATION_SPEC = lambda: ExtraQuery(
    label="traceprov_derivation_spec",
    query=f"$INLINE-select * from traceprov_derivation_spec();",
    runs_after_base=True,
    strict_run=True,
    capture_output=True,
)

TRACEPROV_GET_GENERIC_DERIVATION_SPEC = lambda: ExtraQuery(
    label="traceprov_generic_derivation_spec",
    query=f"$INLINE-select * from traceprov_get_generic_derivation_spec();",
    runs_after_base=True,
    strict_run=True,
    capture_output=True,
)

TRACEPROV_INFER_SPEC = lambda: ExtraQuery(
    label="traceprov_infer",
    query=TP_SKIPPABLE_OPTION,
    func=CallableRepr(
        get_traceprov_extra_infer_func(True), "traceprov_extra_infer_func"
    ),
    repeat=3,
    runs_after_base=True,
)

TRACEPROV_PREPARE_INFER_SPEC = lambda: ExtraQuery(
    label="traceprov_prepare_infer",
    query=TP_SKIPPABLE_OPTION,
    func=CallableRepr(
        get_traceprov_extra_infer_func(False), "traceprov_prepare_extra_infer_func"
    ),
    runs_after_base=True,
)


def traceprov_dump_safe_results(suff: str, results: Any):
    current_timestamp = datetime.now()
    datetime_string = current_timestamp.strftime("%Y_%m_%d_%H_%M_%S")
    result_dir = Path(f"results/{suff}_{datetime_string}/")
    os.makedirs(result_dir, exist_ok=True)
    with open(result_dir / "result.json", "w") as f:
        f.write(json.dumps(results))


def make_traceprov_drop_table_extra(table_name: str):
    return lambda: ExtraQuery(
        label=f"traceprov_drop_table_{table_name}",
        query=f"$INLINE-DROP TABLE IF EXISTS {table_name};",
        runs_after_base=True,
        strict_run=True,
        skip_validation=True,
    )
