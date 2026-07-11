import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.extract_gprom_simple import GPROM_OPTIONS_MAPPING
from traceprovpy.tools.run_duckdb_generic import run_single_query
from traceprovpy.utils import get_filter_group
from utils import make_replacer
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import get_tmp_file, json_read_file, just_read, just_write

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

    g_results = []
    for query_dir in config["dirs"]:

        def run(raw_top_k_limit):
            top_k_limit = get_filter_group(parsed.num_groups, raw_top_k_limit, "post")
            replacer = make_replacer(str(query_dir["num_rows"]), top_k_limit)
            all_results = dict()
            for gprom_mode in GPROM_OPTIONS_MAPPING:
                in_q_dir = Path("./queries/")
                if parsed.optimized:
                    ifile = in_q_dir / f"post_{gprom_mode}.gprom_optimized.sql"
                else:
                    ifile = in_q_dir / f"post_{gprom_mode}.gprom.sql"
                query_contents = just_read(ifile)
                ofile = just_write(tmp_path / "out.sql", replacer(query_contents))
                result = run_single_query(ofile, parsed, total_iters)
                all_results[gprom_mode] = result
            return dict(raw_top_k_limit=raw_top_k_limit, results=all_results)

        for raw_top_k_value in query_dir["sel"]:
            g_results.append(
                dict(dir=query_dir["num_rows"], result=run(raw_top_k_value))
            )
    traceprov_dump_safe_results(
        parsed.suff, dict(result=g_results, optimized=parsed.optimized)
    )


if __name__ == "__main__":
    run()
