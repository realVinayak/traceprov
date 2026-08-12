import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import (
    get_gprom_candidates,
    traceprov_dump_safe_results,
)
from traceprovpy.tools.extract_gprom_simple import (
    GPROM_OPTIONS_MAPPING,
    gprom_add_predicates,
    gprom_handle_rewrite_single_row_mode,
)
from traceprovpy.tools.run_duckdb_generic import (
    run_single_query,
    run_single_query_dry,
    run_option,
)
from traceprovpy.utils import get_filter_group
from utils import make_replacer
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import (
    get_tmp_file,
    json_read_file,
    just_read,
    just_write,
    traceprov_assert_safe_run,
)


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("--num_groups", required=True, type=int)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    # because we won't do any other meanigful comparison anyways.
    parsed.optimized = True
    config: dict = json_read_file(parsed.config)
    assert config is not None

    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    tmp_path = Path(get_tmp_file())
    os.makedirs(tmp_path, exist_ok=True)
    assert parsed.optimized

    g_results = []
    in_q_dir = Path("./queries/")
    for query_dir in config["dirs"]:

        def run_all(raw_top_k_limit):
            top_k_limit = get_filter_group(parsed.num_groups, raw_top_k_limit, "post")
            replacer = make_replacer(str(query_dir["num_rows"]), top_k_limit)
            all_results = dict()
            for gprom_option in get_gprom_candidates("all"):
                gprom_mode = gprom_option.safe_key(True)
                if parsed.optimized:
                    ifile = in_q_dir / f"post_{gprom_mode}.gprom_optimized.sql"
                else:
                    ifile = in_q_dir / f"post_{gprom_mode}.gprom.sql"
                query_contents = just_read(ifile)
                ofile = just_write(tmp_path / "out.sql", replacer(query_contents))
                result = run_single_query(ofile, parsed, total_iters)
                all_results[gprom_mode] = result
            return dict(raw_top_k_limit=raw_top_k_limit, results=all_results)

        def run_offset(raw_top_k_limit):
            top_k_limit = get_filter_group(parsed.num_groups, raw_top_k_limit, "post")
            replacer = make_replacer(str(query_dir["num_rows"]), top_k_limit)
            query_name = f"selectivity_{raw_top_k_limit}"
            orig_base = in_q_dir / "post_base.sql"
            replaced_base = just_write(
                tmp_path / "base_replaced.sql", replacer(just_read(orig_base))
            )
            query_str, query_out_path = gprom_handle_rewrite_single_row_mode(
                query_name, in_q_dir / replaced_base, tmp_path
            )
            all_results = dict()
            initial_exec_str, initial_options = run_single_query_dry(
                query_str.as_posix(), parsed, 1
            )
            traceprov_assert_safe_run(
                f"{initial_exec_str} {initial_options.serialize()}"
            )
            repeat_options = initial_options._replace(repeat=total_iters)
            for gprom_option in get_gprom_candidates("all"):
                gprom_mode = gprom_option.safe_key(True)
                ifile = in_q_dir / f"post_{gprom_mode}.gprom_keys.sql"
                query_contents = just_read(ifile)
                safe_query_contents = replacer(query_contents)
                rewritten_files = gprom_add_predicates(
                    f"selectivity_{raw_top_k_limit}",
                    safe_query_contents,
                    query_out_path,
                    tmp_path,
                    ["GROUP_NUMBER"],
                )
                options_to_run = [
                    repeat_options._replace(i=rf.as_posix()) for rf in rewritten_files
                ]
                offset_results = []
                for offset_id, option_to_run in enumerate(options_to_run):
                    offset_result = run_option(initial_exec_str, option_to_run)
                    offset_results.append(dict(offset=offset_id, result=offset_result))
                all_results[gprom_mode] = offset_results
            return dict(raw_top_k_limit=raw_top_k_limit, results=all_results)

        run_func = run_offset if parsed.single_row_mode else run_all
        for raw_top_k_value in query_dir["sel"]:
            g_results.append(
                dict(dir=query_dir["num_rows"], result=run_func(raw_top_k_value))
            )

    traceprov_dump_safe_results(
        parsed.suff, dict(result=g_results, optimized=parsed.optimized)
    )


if __name__ == "__main__":
    run()
