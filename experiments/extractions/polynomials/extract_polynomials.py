from pathlib import Path
import re
from typing import Literal, NamedTuple

from traceprovpy.tools.extract_query_results import (
    NULL_STATS,
    TpchRow,
    _get_pg_traceprov_handle,
    _get_provsql_handle,
    _handle_pg_gprom,
    dump_rows_list,
    extract_pg_mult,
    get_first_value,
    handle_duckdb_result,
)
from traceprovpy.tools.file_utils import json_read_file, set_query_num
from traceprovpy.tools.normalized_row import ResultExtractor


class MockKeyFactor(NamedTuple):
    factor_value: int
    is_factor: bool

    def get_query_id():
        return ("query_num_factor_value", "query_num_is_factor")


class MockKeyJoin(NamedTuple):
    num_rows: int
    join_count: int

    def get_query_id():
        return ("query_num_num_rows", "query_num_join_count")


def get_query_id(task):
    if task == "factor":
        return MockKeyFactor.get_query_id()
    if task == "join":
        return MockKeyJoin.get_query_id()

    assert False, f"Got unexpected tasl: {task}"


def is_factor(factor_str):
    factor_str = factor_str.lower()
    if factor_str == "factor":
        return True
    if factor_str == "non_factor":
        return False
    assert False, f"Got factor str: {factor_str}"


def rekey_postgres_traceprov_factor(
    factor_mode, factor_result, possible_methods=("base", "traceprov")
):
    result_map = dict()
    for key, result in factor_result.items():
        normal_key = tuple(key.split("_")[-2:])
        method, factor_value = normal_key
        query_name = MockKeyFactor(int(factor_value), is_factor(factor_mode))
        current_dict = result_map.get(query_name, dict())
        assert method in possible_methods, f"Got method {method}"
        assert method not in current_dict, f"Expected {method} to not be in result!"
        current_dict[method] = result
        result_map[query_name] = current_dict
    return result_map


def rekey_postgres_traceprov_join(
    join_mode, join_result, possible_methods=("base", "traceprov")
):
    result_map = dict()
    join_num_count = int(join_mode.split("_")[0])
    for key, result in join_result.items():
        normal_key = tuple(key.split("_"))
        method, num_rows = normal_key
        query_name = MockKeyJoin(int(num_rows), join_num_count - 1)
        current_dict = result_map.get(query_name, dict())
        assert method in possible_methods, f"Got method {method}"
        assert method not in current_dict, f"Expected {method} to not be in result!"
        current_dict[method] = result
        result_map[query_name] = current_dict
    return result_map


def rekey_postgres_provsql(factor_mode, factor_result):
    result_map = dict()
    for key, result in factor_result.items():
        match = re.search(r"provsql_(\d+)['\"]?,?\)_([^']+)$", key)
        assert match
        factor_value = int(match.group(1))
        method = match.group(2)
        assert method in ("phase_1", "phase_1_2")
        query_name = MockKeyFactor(factor_value, is_factor(factor_mode))
        current_dict = result_map.get(query_name, dict())
        assert method not in current_dict, f"Expected {method} to not be in result!"
        if method == "phase_1_2":
            for et in result["extras"]:
                if "capture_and_backtrace" not in et:
                    assert "capture" in et, f"Got {et.keys()}"
                    et["capture_and_backtrace"] = et["capture"]
        current_dict[method] = result
        result_map[query_name] = current_dict
    return result_map


def rekey_postgres_provsql_join(join_mode, join_result):
    result_map = dict()
    for key, result in join_result.items():
        match = re.search(r"provsql_(\d+)_(.+)$", key)
        assert match
        num_rows = int(match.group(1))
        method = match.group(2)
        join_count = int(join_mode.split("_")[0])
        assert method in ("phase_1", "phase_1_2")
        query_name = MockKeyJoin(num_rows, join_count - 1)
        current_dict = result_map.get(query_name, dict())
        assert method not in current_dict, f"Expected {method} to not be in result!"
        current_dict[method] = result
        result_map[query_name] = current_dict
    return result_map


CALL_MODE = Literal["factor"] | Literal["join"]


class PgHandler:
    def traceprov(all_result, version_counter, call_mode: CALL_MODE):
        factor_rows = []
        traceprov_raw_args = dict(
            category=None,
            parallel=1,
            version=version_counter,
            fail_reason="",
            extra=NULL_STATS,
            layer_number=0,
        )
        rekey_func = (
            rekey_postgres_traceprov_factor
            if call_mode == "factor"
            else rekey_postgres_traceprov_join
        )
        for parent_category, parent_result in all_result.items():
            rekeyed = rekey_func(parent_category, parent_result)
            for result in rekeyed.items():
                query_num = result[0]
                query_result = result[1]
                for category, query_category_result in query_result.items():
                    phase_1_profile, phase_1 = extract_pg_mult(
                        query_category_result["base"]
                    )
                    extras = query_category_result["extras"]
                    phase_2 = None
                    phase_2_profile = None
                    if extras:
                        extra_results = [
                            et["traceprov_get_polynomial"][0] for et in extras
                        ]
                        phase_2_profile, phase_2 = extract_pg_mult(extra_results)
                    tp_args = dict(
                        category=category,
                        query_num=query_num,
                        phase_1=phase_1,
                        phase_1_profile=phase_1_profile,
                        phase_2=phase_2,
                        phase_2_profile=phase_2_profile,
                        log_sizes=None,
                    )
                    factor_rows.append(TpchRow(**{**traceprov_raw_args, **tp_args}))
        return factor_rows

    def gprom(factorization_result, version_counter, call_mode: CALL_MODE):
        factor_rows = []
        possible_methods = ("gprom", "base")
        rekey_func = (
            rekey_postgres_traceprov_factor
            if call_mode == "factor"
            else rekey_postgres_traceprov_join
        )
        for factor, factor_result in factorization_result.items():
            rekeyed_result = rekey_func(factor, factor_result, possible_methods)
            for qkey, qvalue in rekeyed_result.items():
                nested_result = dict(gprom=get_first_value(qvalue))
                factor_rows.extend(
                    _handle_pg_gprom((qkey, nested_result), "all", version_counter, 1)
                )
        return factor_rows

    def provsql(factorization_result, version_counter, call_mode: CALL_MODE):
        factor_rows = []
        rekey_func = (
            rekey_postgres_provsql
            if call_mode == "factor"
            else rekey_postgres_provsql_join
        )
        provsql_handler = _get_provsql_handle("provsql", version_counter, 1)[0]
        for factor, factor_result in factorization_result.items():
            rekeyed_result = rekey_func(factor, factor_result)
            print("Final keys: ", rekeyed_result.keys())
            factor_rows.extend(
                mapped
                for query, query_result in rekeyed_result.items()
                for mapped in provsql_handler((query, query_result))
            )

        return factor_rows


def rekey_duckdb_traceprov_factor(factor_results):
    factor_mode = factor_results["factor"]
    mapping = dict()
    for factor_result, factor_result_items in factor_results["result"][
        "results"
    ].items():
        key = MockKeyFactor(1 << int(factor_result), factor_mode)
        mapping[key] = dict(result=factor_result_items)
    return mapping


def rekey_duckdb_traceprov_join(join_results):
    mapping = dict()
    num_joins = join_results["num_joins"]
    for num_rows, num_results in join_results["results"]["results"].items():
        key = MockKeyJoin(int(num_rows), num_joins)
        mapping[key] = dict(result=num_results)
    return mapping


# TODO: There is a LOOOOT of redundancy here.
# SImplify in fututure.
class DuckDBHandler:

    def traceprov(all_combined_result, version_counter, call_mode: CALL_MODE):
        return DuckDBHandler._generic_normal_handler(
            "traceprov", all_combined_result, version_counter, call_mode
        )

    def smokedduck(all_combined_result, version_counter, call_mode: CALL_MODE):
        return DuckDBHandler._generic_normal_handler(
            "smokedduck", all_combined_result, version_counter, call_mode
        )

    def _generic_normal_handler(
        db_system, all_combined_result, version_counter, call_mode: CALL_MODE
    ):
        if call_mode == "join":
            rows = []
            join_results = all_combined_result["results"]["result"]["join"]
            call_options = all_combined_result["call_options"]
            result_mapping = dict()
            for join_result in join_results:
                result_mapping = {
                    **result_mapping,
                    **rekey_duckdb_traceprov_join(join_result),
                }
            combined_mapping = dict(results=result_mapping, call_options=call_options)
            handle_duckdb_result(
                db_system, "all", combined_mapping, None, rows, version_counter
            )
            return rows

        if call_mode == "factor":
            rows = []
            factor_results = all_combined_result["results"]["result"]["factorization"]
            call_options = all_combined_result["call_options"]
            result_mapping = dict()
            for factor_result in factor_results:
                result_mapping = {
                    **result_mapping,
                    **rekey_duckdb_traceprov_factor(factor_result),
                }
            combined_mapping = dict(results=result_mapping, call_options=call_options)
            handle_duckdb_result(
                db_system, "all", combined_mapping, None, rows, version_counter
            )
            return rows
        assert False, "didn't expect to get here!"

    def gprom(all_combined_result, version_counter, call_mode: CALL_MODE):
        if call_mode == "join":
            rows = []
            join_results = all_combined_result["results"]["result"]["join"]
            call_options = all_combined_result["call_options"]
            result_mapping = dict()
            for join_result in join_results:
                result_mapping = {
                    **result_mapping,
                    **rekey_duckdb_traceprov_join(join_result),
                }
            combined_mapping = dict(
                results={
                    q: dict(gprom=[dict(type="success", infer=qval["result"])])
                    for (q, qval) in result_mapping.items()
                },
                call_options=call_options,
            )
            handle_duckdb_result(
                "gprom", "all", combined_mapping, None, rows, version_counter
            )
            return rows

        if call_mode == "factor":
            rows = []
            factor_results = all_combined_result["results"]["result"]["factorization"]
            call_options = all_combined_result["call_options"]
            result_mapping = dict()
            for factor_result in factor_results:
                result_mapping = {
                    **result_mapping,
                    **rekey_duckdb_traceprov_factor(factor_result),
                }
            combined_mapping = dict(
                results={
                    q: dict(gprom=[dict(type="success", infer=qval["result"])])
                    for (q, qval) in result_mapping.items()
                },
                call_options=call_options,
            )
            handle_duckdb_result(
                "gprom", "all", combined_mapping, None, rows, version_counter
            )
            return rows
        assert False, "didn't expect to get here!"


def make_empty_result():
    return dict(factor=[], join=[])


def run():
    result_extractor = ResultExtractor()
    parsed = result_extractor.parse_args()

    out_dir = Path(parsed.out_dir)
    dirs, file_dirs = result_extractor.extract_notes()
    if parsed.dry_run:
        return
    assert parsed.dir is not None
    main_dir = Path(parsed.dir)

    version_counter = 0
    all_result_map = dict()
    for current_dir, current_dir_files in zip(dirs, file_dirs, strict=True):
        backend_system, db_system = current_dir
        if backend_system not in all_result_map:
            all_result_map[backend_system] = make_empty_result()
        target_result = all_result_map[backend_system]
        for current_file in current_dir_files:
            version_counter += 1
            if backend_system == "postgres":
                current_result = json_read_file(
                    main_dir / current_file / "main_result.json"
                )
                assert current_result is not None
                if not hasattr(PgHandler, db_system):
                    print("Skipping: ", db_system)
                    continue
                handler = getattr(PgHandler, db_system)
                target_result["factor"].extend(
                    handler(
                        current_result["result"]["factorization"],
                        version_counter,
                        "factor",
                    )
                )
                target_result["join"].extend(
                    handler(current_result["result"]["join"], version_counter, "join")
                )
            if backend_system == "duckdb":
                current_result = json_read_file(main_dir / current_file / "result.json")
                assert current_result is not None
                if not hasattr(DuckDBHandler, db_system):
                    continue
                handler = getattr(DuckDBHandler, db_system)
                target_result["factor"].extend(
                    handler(current_result, version_counter, "factor")
                )
                target_result["join"].extend(
                    handler(current_result, version_counter, "join")
                )
    for backend_name, backend_result in all_result_map.items():
        for task_name, rows in backend_result.items():
            set_query_num(rows)
            dump_rows_list(
                task_name, backend_name, "all", rows, out_dir, get_query_id(task_name)
            )


if __name__ == "__main__":
    run()
