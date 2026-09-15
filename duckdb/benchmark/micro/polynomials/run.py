import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import (
    get_tmp_file,
    json_read_file,
    just_read,
    just_write,
)
from traceprovpy.tools.run_duckdb_generic import (
    TP_USE_EXTRA,
    create_base_offset,
    run_combined,
    run_single,
    sd_set_preprocessor,
    set_extra_traceprov_options,
    set_extra_traceprov_sample_options,
    tp_use_extra_infer_set,
)

# def set_extra(parsed, table_count):
#     extra_options = {
#         "custom_graph_type": 1,
#         "log_chain_table_count": table_count,
#         "min_layer_number": 2,
#     }
#     flat = [f"--{key} {value}" for (key, value) in extra_options.items()]
#     combined = " ".join(flat)
#     set_extra_traceprov_options(parsed, combined)


def run_factorized_traceprov(
    config, parsed, tmp: Path, total_iters, factor: bool = False
):
    factor_label = "factor" if factor else "non_factor"
    factorization = config["factorization"]
    # run all the factor ones first
    query_dir = Path(f"./factorization/{factor_label}/")

    def make_replacer(factor_value: int):
        def _replacer(in_sql: str):
            assert "NUM" in in_sql
            query_str = in_sql.replace("NUM", str(1 << factor_value))
            return query_str

        return _replacer

    out_dir = tmp
    os.makedirs(out_dir, exist_ok=True)

    query = factor_label
    parsed.graph_dir = Path("./") / "graphs"
    all_factor_results = dict(type=factor_label, results=dict())
    factor_results = all_factor_results["results"]
    parsed.base_root = out_dir
    parsed.root = out_dir
    query_out_dir = out_dir / query
    sd_mode = "lineage_view" if parsed.sample_inference is None else "lineage_query"
    sd_path = query_dir / f"sd_{sd_mode}_polynomial.sql"
    sd_polynomial_query = just_read(sd_path) if sd_path.exists() else None
    preprocessor = lambda output_id, in_query: (
        in_query
        if sd_polynomial_query is None
        else sd_polynomial_query.replace("OUT_ID", str(output_id))
    )
    sd_set_preprocessor(parsed, preprocessor)
    os.makedirs(query_out_dir, exist_ok=True)
    just_write(query_out_dir / "sd_thread_1_combined.sql", (sd_polynomial_query))
    for factor in factorization:
        base_sql = just_read(query_dir / "base.sql")
        create_base_offset(query_dir)
        base_offset_sql = just_read(query_dir / "base_offset.sql")
        capture_new_compact_sql = just_read(query_dir / "capture_new_compact.sql")
        print("Capture SQL: ", capture_new_compact_sql)
        traceprov_extra = [query_dir / "traceprov.sql"]
        # traceprov_sql = just_read(query_dir / "traceprov.sql")
        replacer = make_replacer(factor)
        just_write(query_out_dir / "base.sql", replacer(base_sql))
        just_write(query_out_dir / "base_offset.sql", replacer(base_offset_sql))
        just_write(
            query_out_dir / "capture_new_compact.sql",
            replacer(capture_new_compact_sql),
        )

        # parsed.infer = False
        tp_use_extra_infer_set(parsed, traceprov_extra)
        if parsed.sd_mode is None:
            set_extra_traceprov_options(parsed, " --mock_traceprov_bt_data ")
        else:
            extra_options = ["disable_chunk_cache"]
            # if query in needs_perfect_hash_disable:
            #     extra_options.append("disable_perfect_hash")
            extra_options = " ".join([f"--{key}" for key in extra_options])
            set_extra_traceprov_options(parsed, extra_options)
        traceprov_result = run_combined(parsed, total_iters, query, [], 5)
        set_extra_traceprov_options(parsed, "")
        tp_use_extra_infer_set(parsed, [])
        factor_results[factor] = traceprov_result
    sd_set_preprocessor(parsed, None)
    return factor_results


def run_join_traceprov(config, parsed, tmp: Path, total_iters, raw_num_joins: int):
    orig_raw_num_joins = raw_num_joins
    raw_num_joins += 1
    num_join_label = str(raw_num_joins)
    # # factor_label = "factor" if factor else "non_factor"
    join_table_size = config["simple_join"]
    # run all the factor ones first
    query_dir = Path(f"./join/{raw_num_joins}_way_join/")

    def make_replacer(factor_value: int):
        def _replacer(in_sql: str):
            assert "NUM" in in_sql
            query_str = in_sql.replace("NUM", str(factor_value))
            return query_str

        return _replacer

    out_dir = tmp
    os.makedirs(out_dir, exist_ok=True)

    query = f"{num_join_label}_way_join"
    parsed.graph_dir = Path("./") / "graphs"
    all_factor_results = dict(type=num_join_label, results=dict())
    factor_results = all_factor_results["results"]
    parsed.base_root = out_dir
    parsed.root = out_dir
    query_out_dir = out_dir / query
    sd_mode = "lineage_view" if parsed.sample_inference is None else "lineage_query"
    sd_path = query_dir / f"sd_{sd_mode}_polynomial.sql"
    sd_polynomial_query = just_read(sd_path) if sd_path.exists() else None
    preprocessor = lambda output_id, in_query: (
        in_query
        if sd_polynomial_query is None
        else sd_polynomial_query.replace("OUT_ID", str(output_id))
    )
    sd_set_preprocessor(parsed, preprocessor)
    os.makedirs(query_out_dir, exist_ok=True)
    for table_size in join_table_size:
        sd_combined_path = query_out_dir / "sd_thread_1_combined.sql"
        if sd_polynomial_query:
            just_write(sd_combined_path, (sd_polynomial_query))
        base_sql = just_read(query_dir / "base.sql")
        create_base_offset(query_dir)
        base_offset_sql = just_read(query_dir / "base_offset.sql")
        capture_new_compact_sql = just_read(query_dir / "capture_new_compact.sql")
        print("Capture SQL: ", capture_new_compact_sql)
        traceprov_extra = [query_dir / "traceprov.sql"]
        # traceprov_sql = just_read(query_dir / "traceprov.sql")
        replacer = make_replacer(table_size)
        just_write(query_out_dir / "base.sql", replacer(base_sql))
        just_write(query_out_dir / "base_offset.sql", replacer(base_offset_sql))
        just_write(
            query_out_dir / "capture_new_compact.sql",
            replacer(capture_new_compact_sql),
        )
        # parsed.infer = False
        tp_use_extra_infer_set(parsed, traceprov_extra)
        extra_file_out = tmp / "extra_offsets.txt"
        traceprov_extra_expanded = []
        current_query = just_read(traceprov_extra[0])
        # for file in just_read(extra_file_out):
        if parsed.sd_mode is None:
            if parsed.sample_inference is None:
                for output_id in range((table_size)):
                    replaced_query = current_query.replace(";", "")
                    if parsed.mat_infer:
                        replaced_query = f"create or replace table traceprov_out_{output_id} as ({replaced_query})"
                    traceprov_extra_expanded.append(
                        just_write(
                            tmp / f"traceprov_backtrace_{output_id}.sql", replaced_query
                        )
                    )
                traceprov_extra_expanded = list(map(str, traceprov_extra_expanded))
                extra_out = just_write(
                    extra_file_out, "\n".join(traceprov_extra_expanded)
                )
                set_extra_traceprov_sample_options(parsed, f"--extra_file {extra_out}")
            set_extra_traceprov_options(parsed, f" --mock_traceprov_bt_data")
        else:
            extra_options = ["disable_chunk_cache"]
            # if query in needs_perfect_hash_disable:
            #     extra_options.append("disable_perfect_hash")
            extra_options = " ".join([f"--{key}" for key in extra_options])
            set_extra_traceprov_options(parsed, extra_options)
        traceprov_result = run_combined(parsed, total_iters, query, [], 2)
        set_extra_traceprov_options(parsed, "")
        tp_use_extra_infer_set(parsed, [])
        factor_results[table_size] = traceprov_result
    sd_set_preprocessor(parsed, None)
    return factor_results


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
    factorization_result = run_factorized_traceprov(
        config, parsed, tmp, total_iters, True
    )
    non_factorization_result = run_factorized_traceprov(
        config, parsed, tmp, total_iters, False
    )
    # factorization_result = None
    # non_factorization_result = None
    all_results["factorization"] = [factorization_result, non_factorization_result]
    join_results = []
    for num_joins in config["num_joins"]:
        join_results.append(
            dict(
                num_joins=num_joins,
                results=run_join_traceprov(config, parsed, tmp, total_iters, num_joins),
            )
        )
    all_results["join"] = join_results
    # all_results["factorization"] = factorization_result
    traceprov_dump_safe_results(parsed.suff, dict(result=all_results))


if __name__ == "__main__":
    run()
