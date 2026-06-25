# the infer func.

import getpass
import json
from pathlib import Path
from traceprovpy.tools.benchmark import OPTION_GETTER, ExtraQuery, QuerySpec
from traceprovpy.tools.file_utils import just_read, traceprov_assert_safe_run
from traceprovpy.tools.run_with_timeout import RunWithTimeoutOptions, run_with_timeout

TRACEPROV_LAYERS_TO_DERIVE_KEY = "TRACEPROV_LAYERS_TO_DERIVE_KEY"
TRACEPROV_MATERIALIZE_LAYER_KEY = "TRACEPROV_MATERIALIZE_LAYER_KEY"
TRACEPROV_DERIVE_OFFSET_KEY = "TRACEPROV_DERIVE_BULK"
TRACEPROV_PROFILE_DUCKDB = "TRACEPROV_PROFILE_DUCKDB"

TRACEPROV_INFER_SPEC_QUERY = (
    "select * from traceprov_get_generic_derivation_spec(false);"
)
TRACEPROV_PREPARE_FOR_SCAN = (
    lambda offset: f"select * from traceprov_prepare_for_scan({offset});"
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


def get_infer_relation_name(layer: int):
    return f"traceprov_relation_infer_{layer}"


def get_matching_element(table: str, create_layers: str):
    filtered = [
        (idx, layer) for idx, layer in enumerate(create_layers) if f"{table}(" in layer
    ]
    assert len(filtered) == 1
    return filtered[0]


def set_profile_path(cursor, path):
    cursor.execute(f"SET traceprov.duckdb_profile_out='{path}'")


def infer_offsets(current_result: dict):
    complete_plan = json.loads(current_result["complete_plan"])
    row_count = complete_plan["Plan"]["Actual Rows"]
    return list(range(0, row_count))


def get_traceprov_extra_infer_func(perform_inference: bool = True):

    def traceprov_extra_infer_func(args):
        query_spec = args["self"]
        extra_spec = args["extra"]
        run_time_options: RunWithTimeoutOptions = args["extra_pack"]
        getter = args["get_run_options"]
        current_result = args["current_result"]
        assert query_spec.extra_options is not None
        layers_to_derive = query_spec.extra_options[TRACEPROV_LAYERS_TO_DERIVE_KEY]
        print("Deriving layers: ", layers_to_derive)
        is_validate = query_spec.extra_options[TRACEPROV_MATERIALIZE_LAYER_KEY]
        bulk_derive = query_spec.extra_options[TRACEPROV_DERIVE_OFFSET_KEY]
        global_is_last = args["is_global_last"]
        all_offsets = [-1] if bulk_derive else infer_offsets(current_result)

        if not bulk_derive and not global_is_last:
            return dict(type="early")

        total_repeat = (
            extra_spec.repeat
            if bulk_derive
            else (run_time_options.params.repeat + run_time_options.params.throwaway)
        )

        def derive_for_offset(offset):
            capture_duckdb_profile = query_spec.extra_options[TRACEPROV_PROFILE_DUCKDB]
            prepare_for_scan = TRACEPROV_PREPARE_FOR_SCAN(
                "" if offset == -1 else str(offset)
            )
            conn = run_time_options.run_connection_strict()
            cursor = conn.cursor()
            cursor.execute("BEGIN TRANSACTION;")
            duckdb_profile_path = Path("/tmp/") / "traceprov_duckdb_profile.json"
            if capture_duckdb_profile:
                set_profile_path(cursor, duckdb_profile_path.as_posix())
            prepare_cursor_explain = (
                f"{run_time_options.get_explain(conn)} {prepare_for_scan}"
            )
            cursor.execute(prepare_cursor_explain)
            prepare_for_scan_analyze_result = cursor.fetchall()[0][0][0]
            cursor.execute(prepare_for_scan)
            infer_spec = json.loads(cursor.fetchall()[0][0])
            print(infer_spec)
            elements = infer_spec["sql"]
            derivable_layers = list(map(get_infer_relation_name, layers_to_derive))
            filtered_layers = [
                (layer, get_matching_element(layer, elements))
                for layer in derivable_layers
            ]
            assert len(filtered_layers) > 0
            create_table_analyze_results = []
            for table, (_, create_table_sql) in filtered_layers:
                cursor.execute(f"DROP TABLE IF EXISTS {table};")
                cursor.execute(create_table_sql)
                # create_table_explain_sql = (
                #     f"{run_time_options.get_explain(conn)} {create_table_sql}"
                # )
                # cursor.execute(create_table_explain_sql)
                # create_table_analyze_results.append(cursor.fetchall()[0][0][0])
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
                    for _ in range(total_repeat):
                        rsi_result = run_with_timeout(rt_option)
                        # rsi_timings = _run_simple(cursor, TRACEPROV_INFER_QUERY_STAT(idx, False))
                        profile = None
                        if capture_duckdb_profile:
                            username = getpass.getuser()
                            traceprov_assert_safe_run(
                                f"sudo chown {username} {duckdb_profile_path.as_posix()}"
                            )
                            profile = just_read(duckdb_profile_path)
                            traceprov_assert_safe_run(
                                f"rm -f {duckdb_profile_path.as_posix()}"
                            )
                        result_elem["raw_results"].append(
                            dict(result=rsi_result, timings=None, profile=profile)
                        )
                    cursor.execute(f"select count(*) from {layer};")
                    result_elem["row_count"] = cursor.fetchall()[0][0]
                    # also compute the row count.
                    results.append(result_elem)

            if capture_duckdb_profile:
                set_profile_path(cursor, "")

            cursor.execute("COMMIT;")
            cursor.close()

            offset_results = dict(
                prepare_for_scan_init=prepare_for_scan_analyze_result,
                core_results=results,
                create_table_analyze_results=create_table_analyze_results,
            )
            return offset_results

        final_results = list(
            [
                dict(offset=offset, offset_result=derive_for_offset(offset))
                for offset in all_offsets
            ]
        )
        return dict(type="success", results=final_results)

    return traceprov_extra_infer_func
