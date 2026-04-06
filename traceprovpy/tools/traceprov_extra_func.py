# the infer func.

import json
from pathlib import Path
from traceprovpy.tools.benchmark import OPTION_GETTER, ExtraQuery, QuerySpec
from traceprovpy.tools.run_with_timeout import RunWithTimeoutOptions, run_with_timeout

TRACEPROV_INFER_SPEC_QUERY = (
    "select * from traceprov_get_generic_derivation_spec(false);"
)

TRACEPROV_INFER_QUERY = (
    lambda idx, cols: f"select * from traceprov_perform_duckdb_inference_fast({idx}) as {cols}"
)

TRACEPROV_INFER_QUERY_STAT = (
    lambda idx, perform_infer: f"select * from traceprov_get_infer_stat({idx}, {perform_infer})"
)


def make_cols(width: int):
    aliases = ",".join((f"col_{idx} bigint" for idx in range(width)))
    return f"({aliases})"


def _run_simple(cursor, query):
    cursor.execute(query)
    return cursor.fetchall()


def traceprov_extra_infer_func(
    query_spec: QuerySpec,
    extra_spec: ExtraQuery,
    run_time_options: RunWithTimeoutOptions,
    getter: OPTION_GETTER,
):
    conn = run_time_options.run_connection_strict()
    cursor = conn.cursor()
    cursor.execute(TRACEPROV_INFER_SPEC_QUERY)
    infer_spec = json.loads(cursor.fetchall()[0][0])
    print(infer_spec)
    elements = infer_spec["elements"]
    results = []

    for element in elements:
        idx = element["idx"]
        width = element["expected_width"]
        rt_option = query_spec.get_pack(
            Path("/tmp/"),
            f"$INLINE-{TRACEPROV_INFER_QUERY(idx, make_cols(width))};",
            getter,
        )
        assert isinstance(rt_option, RunWithTimeoutOptions)
        print(rt_option)
        rt_option = rt_option._replace(extras=run_time_options.extras)
        result_elem = dict(id=idx, rsi_results=[], infer_results=[])
        for _ in range(extra_spec.repeat):
            rsi_result = run_with_timeout(rt_option)
            rsi_timings = _run_simple(cursor, TRACEPROV_INFER_QUERY_STAT(idx, False))
            result_elem["rsi_results"].append(
                dict(result=rsi_result, timings=rsi_timings)
            )
        for _ in range(extra_spec.repeat):
            result_elem["infer_results"].append(
                dict(timings=_run_simple(cursor, TRACEPROV_INFER_QUERY_STAT(idx, True)))
            )
        results.append(result_elem)
    cursor.close()
    return results
