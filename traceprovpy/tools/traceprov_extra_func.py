# the infer func.

import json
from pathlib import Path
from traceprovpy.tools.benchmark import OPTION_GETTER, ExtraQuery, QuerySpec
from traceprovpy.tools.run_with_timeout import RunWithTimeoutOptions, run_with_timeout

TRACEPROV_LAYERS_TO_DERIVE_KEY = "TRACEPROV_LAYERS_TO_DERIVE_KEY"
TRACEPROV_MATERIALIZE_LAYER_KEY = "TRACEPROV_MATERIALIZE_LAYER_KEY"

TRACEPROV_INFER_SPEC_QUERY = (
    "select * from traceprov_get_generic_derivation_spec(false);"
)
TRACEPROV_PREPARE_FOR_SCAN = "select * from traceprov_prepare_for_scan();"

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


def get_infer_relation_name(layer: int):
    return f"traceprov_relation_infer_{layer}"


def get_matching_element(table: str, create_layers: str):
    filtered = [
        (idx, layer) for idx, layer in enumerate(create_layers) if f"{table}(" in layer
    ]
    assert len(filtered) == 1
    return filtered[0]


def get_traceprov_extra_infer_func(perform_inference: bool = True):

    def traceprov_extra_infer_func(
        query_spec: QuerySpec,
        extra_spec: ExtraQuery,
        run_time_options: RunWithTimeoutOptions,
        getter: OPTION_GETTER,
    ):
        assert query_spec.extra_options is not None
        layers_to_derive = query_spec.extra_options[TRACEPROV_LAYERS_TO_DERIVE_KEY]
        is_validate = query_spec.extra_options[TRACEPROV_MATERIALIZE_LAYER_KEY]
        conn = run_time_options.run_connection_strict()
        cursor = conn.cursor()
        cursor.execute(TRACEPROV_PREPARE_FOR_SCAN)
        infer_spec = json.loads(cursor.fetchall()[0][0])
        print(infer_spec)
        elements = infer_spec["sql"]
        derivable_layers = list(map(get_infer_relation_name, layers_to_derive))
        filtered_layers = [
            (layer, get_matching_element(layer, elements)) for layer in derivable_layers
        ]
        assert len(filtered_layers) > 0
        for table, (_, create_table_sql) in filtered_layers:
            cursor.execute(f"DROP TABLE IF EXISTS {table};")
            cursor.execute(create_table_sql)

            if not is_validate:
                continue

            new_table = f"{table}_mat"
            create_table_sql = (
                f"create table {new_table} USING heap as (select * from {table})"
            )
            cursor.execute(f"DROP TABLE IF EXISTS {new_table};")
            cursor.execute(create_table_sql)

        results = []

        if perform_inference:

            for layer, _ in filtered_layers:
                rt_option = query_spec.get_pack(
                    Path("/tmp/"),
                    f"$INLINE-SELECT * FROM {layer};",
                    getter,
                )
                assert isinstance(rt_option, RunWithTimeoutOptions)
                print(rt_option)
                rt_option = rt_option._replace(extras=run_time_options.extras)
                result_elem = dict(layer=layer, raw_results=[], row_count=-1)
                for _ in range(extra_spec.repeat):
                    rsi_result = run_with_timeout(rt_option)
                    # rsi_timings = _run_simple(cursor, TRACEPROV_INFER_QUERY_STAT(idx, False))
                    result_elem["raw_results"].append(
                        dict(result=rsi_result, timings=None)
                    )
                cursor.execute(f"select count(*) from {layer};")
                result_elem["row_count"] = cursor.fetchall()[0][0]
                # also compute the row count.
                results.append(result_elem)
        cursor.close()
        return results

    return traceprov_extra_infer_func
