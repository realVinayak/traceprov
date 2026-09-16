import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from utils import (
    create_or_replace_table,
    make_factor_replacer,
    make_join_replacer,
)
from traceprovpy.tools.file_utils import (
    get_tmp_file,
    json_read_file,
    just_read,
    just_write,
)
from traceprovpy.tools.run_duckdb_generic import run_single_query


def run_factorized_gprom(config, parsed, tmp: Path, total_iters, factor: bool = False):
    factor_label = "factor" if factor else "non_factor"
    factorization = config["factorization"]
    # run all the factor ones first
    query_dir = Path(f"./factorization/{factor_label}/")

    out_dir = tmp
    os.makedirs(out_dir, exist_ok=True)

    query = factor_label
    all_factor_results = dict(type=factor_label, results=dict())
    factor_results = all_factor_results["results"]
    parsed.base_root = out_dir
    parsed.root = out_dir
    gprom_sql = just_read(query_dir / "gprom.sql")
    query_out_dir = out_dir / query
    os.makedirs(query_out_dir, exist_ok=True)
    for factor in factorization:
        replacer = make_factor_replacer(factor)
        current_query = gprom_sql
        if parsed.mat_infer:
            current_query = create_or_replace_table(
                current_query, f"traceprov_gprom_{factor_label}_{factor}"
            )
        gprom_query_file = just_write(
            query_out_dir / "gprom.sql", replacer(current_query)
        )
        result = run_single_query(gprom_query_file, parsed, total_iters)
        factor_results[factor] = result
    return all_factor_results


def run_join_gprom(config, parsed, tmp: Path, total_iters, raw_num_joins: int):
    orig_raw_num_joins = raw_num_joins
    raw_num_joins += 1
    num_join_label = str(raw_num_joins)
    # # factor_label = "factor" if factor else "non_factor"
    join_table_size = config["simple_join"]
    # run all the factor ones first
    query_dir = Path(f"./join/{raw_num_joins}_way_join/")

    out_dir = tmp
    os.makedirs(out_dir, exist_ok=True)

    query = f"{num_join_label}_way_join"

    all_factor_results = dict(type=num_join_label, results=dict())
    factor_results = all_factor_results["results"]
    parsed.base_root = out_dir
    parsed.root = out_dir
    query_out_dir = out_dir / query
    os.makedirs(query_out_dir, exist_ok=True)
    gprom_sql = just_read(query_dir / "gprom.sql")
    for table_size in join_table_size:
        replacer = make_join_replacer(table_size)
        current_query = gprom_sql
        if parsed.mat_infer:
            current_query = create_or_replace_table(
                current_query, f"traceprov_gprom_{raw_num_joins}_{table_size}"
            )
        gprom_query_file = just_write(
            query_out_dir / "gprom.sql", replacer(current_query)
        )
        result = run_single_query(gprom_query_file, parsed, total_iters)
        factor_results[table_size] = result
    return all_factor_results


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    # base_parser.add_argument(
    #     "--sd_query_dir", required=False, default="./join/sd_query_dir/"
    # )
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    parsed.simple_join_mode = True
    config: dict = json_read_file(parsed.config)
    assert config is not None
    # factorization = config["factorization"]
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    tmp = Path(get_tmp_file())
    os.makedirs(tmp, exist_ok=True)
    all_results = dict()
    # if parsed.sd_mode is None:
    factorization_result = run_factorized_gprom(config, parsed, tmp, total_iters, True)
    non_factorization_result = run_factorized_gprom(
        config, parsed, tmp, total_iters, False
    )
    # factorization_result = None
    # non_factorization_result = None
    all_results["factorization"] = [
        dict(factor=True, result=factorization_result),
        dict(factor=False, result=non_factorization_result),
    ]
    join_results = []
    for num_joins in config["num_joins"]:
        join_results.append(
            dict(
                num_joins=num_joins,
                results=run_join_gprom(config, parsed, tmp, total_iters, num_joins),
            )
        )
    all_results["join"] = join_results
    # all_results["factorization"] = factorization_result
    traceprov_dump_safe_results(parsed.suff, dict(result=all_results))


if __name__ == "__main__":
    run()
