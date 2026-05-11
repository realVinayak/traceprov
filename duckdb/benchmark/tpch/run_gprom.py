# gprom driver.
from pathlib import Path
import subprocess
import time

from duckdb import query

from traceprovpy.tools.benchmark_utils import (
    add_gprom_candidates,
    get_gprom_candidates,
    infer_gprom_candidates,
    traceprov_dump_safe_results,
)
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.extract_gprom_simple import GpromOptions
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_duckdb_generic import (
    add_query_options,
    infer_option_results,
    run_single_query,
    run_single_query_dry,
)


def run_possible_queries(
    query_name: str, gprom_mode: str, gprom_config: dict, parsed, iters
):
    valid_specs = infer_gprom_candidates(gprom_mode, gprom_config)
    result = dict()
    queries = dict()
    for valid_spec in valid_specs:
        safe_key = valid_spec.safe_key()
        assert safe_key not in result
        query = Path("./params_default_gprom") / query_name / f"{safe_key}.sql"
        assert query.exists(), f"Expected {query} to exist!"
        queries[safe_key] = query
    for key, query_str in queries.items():
        # first, run it just once with a timeout, to check if it'll finish in timeout or not.
        exec_str, options = run_single_query_dry(query_str.as_posix(), parsed, 1)
        repeat_options = options._replace(repeat=iters)
        options = options._replace(threads=parsed.threads)
        try:
            sub_result = subprocess.run(
                [
                    exec_str,
                    *[x for f in options.get_list_options() for x in f.split(" ")],
                ],
                timeout=300,
            )
        except subprocess.TimeoutExpired:
            print("Detected timeout!")
            result[key] = dict(timeout=True)
            continue
        except Exception as e:
            result[key] = dict(error=str(e))
            continue

        if sub_result.returncode != 0:
            result[key] = dict(errorcode=sub_result.returncode)
            continue

        try:
            later_sub_result = subprocess.run(
                [
                    exec_str,
                    *[
                        x
                        for f in repeat_options.get_list_options()
                        for x in f.split(" ")
                    ],
                ]
            )
        except Exception as e:
            result[key] = dict(later_error=str(e))

        if later_sub_result.returncode != 0:
            result[key] = dict(later_errorcode=later_sub_result.returncode)
            continue

        result[key] = infer_option_results(repeat_options)
    return {query_name: result}


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("-g_cfg", "--gprom_config", required=True, type=str)
    add_gprom_candidates(base_parser)
    # traceprov_handle_suffix(parsed)

    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    config: dict = json_read_file(parsed.config)
    assert config is not None
    gprom_config: dict = json_read_file(parsed.gprom_config)
    assert gprom_config is not None

    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    query_repr = config["queries"]
    if isinstance(query_repr, str):
        query_repr = eval(query_repr)
    query_results = {}
    for query_name in query_repr:
        print(query_name)
        query_name = str(query_name)
        original_query_name = query_name
        if query_name not in gprom_config:
            query_name = query_name.rjust(2, "0")
            if query_name not in gprom_config:
                continue
        g_config_item = gprom_config[query_name]
        result = run_possible_queries(
            original_query_name, parsed.mode, g_config_item, parsed, total_iters
        )
        new_result = {**query_results, **result}
        assert len(new_result) > len(query_results), "Got some duplicated keys!"
        query_results = new_result
    traceprov_dump_safe_results(parsed.suff, query_results)


if __name__ == "__main__":
    run()
