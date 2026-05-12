import argparse
from itertools import product
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.extract_gprom_simple import GPROM_OPTIONS_MAPPING
from traceprovpy.tools.run_duckdb_generic import run_single_query
from utils import get_filter_group, make_replacer
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import json_read_file, just_read, just_write


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("--num_groups", required=True, type=int)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    config: dict = json_read_file(parsed.config)
    assert config is not None

    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    tmp_path = Path("./tmp/")
    os.makedirs(tmp_path, exist_ok=True)

    g_results = []
    for query_dir in config["dirs"]:

        def run(mode, raw_selectivity):
            selectivity = get_filter_group(parsed.num_groups, raw_selectivity, mode)
            replacer = make_replacer(str(query_dir["num_rows"]), selectivity, True)
            all_results = dict()
            for gprom_mode in GPROM_OPTIONS_MAPPING:
                if parsed.optimized:
                    ifile = (
                        Path("./queries/") / f"{mode}_{gprom_mode}.gprom_optimized.sql"
                    )
                else:
                    ifile = Path("./queries/") / f"{mode}_{gprom_mode}.gprom.sql"
                query_contents = just_read(ifile)
                ofile = just_write(tmp_path / "out.sql", replacer(query_contents))
                result = run_single_query(ofile, parsed, total_iters)
                all_results[gprom_mode] = result
            return dict(raw_selectivity=raw_selectivity, mode=mode, results=all_results)

        modes = ["post", "pre"]
        combs = product(modes, query_dir["sel"])
        for comb in combs:
            g_results.append(dict(dir=query_dir["num_rows"], result=run(*comb)))

    traceprov_dump_safe_results(
        parsed.suff, dict(result=g_results, optimized=parsed.optimized)
    )


if __name__ == "__main__":
    run()
