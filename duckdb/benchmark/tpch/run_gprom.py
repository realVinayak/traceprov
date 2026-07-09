# gprom driver.
import json
import os
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
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.extract_gprom_simple import GpromOptions
from traceprovpy.tools.file_utils import (
    get_tmp_file,
    json_read_file,
    just_read,
    just_write,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.run_duckdb_generic import (
    add_query_options,
    infer_option_results,
    run_single_query,
    run_single_query_dry,
)
from traceprovpy.tools.run_with_timeout import MakeKeySelection

def handle_rewrite_single_row_mode(
    query_name: str,
    base_query_path: Path,
    temp_dir: Path,
):
    query = just_read(base_query_path)
    assert query is not None
    query_out_path = temp_dir / "query_out.json"
    query_rewritten = temp_dir / f"query_rewritten_{query_name}.sql"
    query = query.replace(";", "")
    query = f"copy (select * from ({query})) to '{query_out_path.as_posix()}'"
    just_write(query_rewritten, query)
    return query_rewritten, query_out_path


def add_predicates(
    query_name: str,
    query_path: Path,
    result_path: Path,
    temp_dir: Path,
    query_keys: dict,
):
    raw_query_result = just_read(result_path).splitlines()
    assert isinstance(raw_query_result, list)
    query_result = list(map(json.loads, raw_query_result))
    print(query_result)
    query = just_read(query_path)
    files: list[Path] = []
    query_keys = [key.lower() for key in query_keys]
    for row_result_idx, row_result in enumerate(query_result):
        filter_pack = {
            key: value
            for (key, value) in row_result.items()
            if not key.lower().startswith("prov_") and key.lower() in query_keys
        }
        preprocessor = MakeKeySelection(filter_pack)
        filtered = preprocessor.preprocess(query)
        out_dir = temp_dir / str(row_result_idx)
        os.makedirs(out_dir, exist_ok=True)
        query_rewritten_file = out_dir / f"filtered_{query_name}.sql"
        just_write(query_rewritten_file, filtered)
        files.append(query_rewritten_file)
    return files


def run_option(exec_str: str, options: DuckDBDriverOptions):
    result = os.system(f"{exec_str} {options.serialize()}")
    if result != 0:
        return dict(type="fail", code=result)
    return dict(type="sucess", infer=infer_option_results(options))


def run_possible_queries(
    query_name: str,
    gprom_mode: str,
    gprom_config: dict,
    parsed,
    iters,
    gprom_query_dir: Path,
    temp_dir: Path,
    base_query_path: Path,
    keys: dict,
):
    valid_specs = infer_gprom_candidates(gprom_mode, gprom_config)
    result = dict()
    queries = dict()
    for valid_spec in valid_specs:
        safe_key = valid_spec.safe_key()
        assert safe_key not in result
        query = gprom_query_dir / query_name / f"{safe_key}.sql"
        assert query.exists(), f"Expected {query} to exist!"
        queries[safe_key] = query
    for key, query_str in queries.items():
        traceprov_assert_safe_run(f"rm -rf {parsed.db}.tmp/")
        # first, run it just once with a timeout, to check if it'll finish in timeout or not.
        original_query_str = query_str
        if parsed.single_row_mode:
            query_str, query_out_path = handle_rewrite_single_row_mode(
                query_name, base_query_path / query_name / "base.sql", temp_dir
            )
        exec_str, options = run_single_query_dry(query_str.as_posix(), parsed, 1)
        repeat_options = options._replace(repeat=iters)
        options = options._replace(threads=parsed.threads)
        try:
            sub_result = subprocess.run(
                [
                    exec_str,
                    *[x for f in options.get_list_options() for x in f.split(" ")],
                ],
                stderr=subprocess.PIPE,
                text=True,
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
            result[key] = dict(
                errorcode=sub_result.returncode, error_content=sub_result.stderr
            )
            continue

        options_to_run = [repeat_options]
        if parsed.single_row_mode:
            rewritten_files = add_predicates(
                query_name,
                original_query_str,
                query_out_path,
                temp_dir,
                keys[query_name],
            )
            options_to_run = [
                repeat_options._replace(i=rewritten_file.as_posix())
                for rewritten_file in rewritten_files
            ]

        results = []
        for option_to_run in options_to_run:
            option_result = run_option(exec_str, option_to_run)
            results.append(option_result)
            if option_result["type"] == "fail":
                # if there is a failure, early out.
                break
        result[key] = results
    traceprov_assert_safe_run(f"rm -rf {parsed.db}/.tmp/")
    return {query_name: result}


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("--dir", required=True)
    base_parser.add_argument("--temp_dir", required=False, default=get_tmp_file())
    base_parser.add_argument("--base_dir", required=False, default="./")
    keys_path = Path(
        "../../../../postgres/benchmark/tpch/legacy_scale_1/params_default/extract_gprom/keys.json"
    )
    base_parser.add_argument(
        "--keys",
        required=False,
        default=keys_path,
    )
    add_gprom_candidates(base_parser)
    # traceprov_handle_suffix(parsed)

    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    config: dict = json_read_file(parsed.config)
    assert config is not None
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    query_repr = config["queries"]
    if isinstance(query_repr, str):
        query_repr = eval(query_repr)
    query_results = {}
    gprom_query_dir = Path(parsed.dir)
    assert not parsed.optimized or "optimized" in str(gprom_query_dir)

    gprom_config: dict = json_read_file(gprom_query_dir / "config_gprom.json")
    assert gprom_config is not None

    temp_dir = Path(parsed.temp_dir)
    os.makedirs(temp_dir, exist_ok=True)
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
            original_query_name,
            parsed.mode,
            g_config_item,
            parsed,
            total_iters,
            gprom_query_dir,
            temp_dir,
            Path(parsed.base_dir),
            json_read_file(Path(parsed.keys)),
        )
        new_result = {**query_results, **result}
        assert len(new_result) > len(query_results), "Got some duplicated keys!"
        query_results = new_result
    traceprov_dump_safe_results(parsed.suff, query_results)


if __name__ == "__main__":
    run()
