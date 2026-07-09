import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.extract_gprom_simple import GPROM_OPTIONS_MAPPING
from traceprovpy.tools.run_duckdb_generic import run_single_query
from utils import ALL_QUERY_LIST, make_replacer
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import get_tmp_file, json_read_file, just_read, just_write

def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("-cfg", "--config", required=True)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    config: dict = json_read_file(parsed.config)
    assert config is not None
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    tmp_path = Path(get_tmp_file())
    os.makedirs(tmp_path, exist_ok=True)
    g_results = []
    for query_dir in config["dirs"]:
        replacer = make_replacer(int(query_dir["num_rows"]))

        def run(query_name: str):
            all_results = dict()
            for gprom_mode in GPROM_OPTIONS_MAPPING:
                query_dir = Path("./queries/") / query_name
                if parsed.optimized:
                    ifile = query_dir / f"{gprom_mode}_optimized.sql"
                else:
                    ifile = query_dir / f"{gprom_mode}.sql"
                query_contents = replacer(just_read(ifile))
                ofile = just_write(tmp_path / "out.sql", replacer(query_contents))
                result = run_single_query(ofile, parsed, total_iters)
                all_results[gprom_mode] = result
            return dict(query_name=query_name, results=all_results)

        if query_dir["queries"] == "_all_":
            query_list = ALL_QUERY_LIST
        else:
            query_list = query_dir["queries"]
        query_results = {query_name: run(query_name) for query_name in query_list}
        g_results.append(dict(dir=query_dir, results=query_results))
    traceprov_dump_safe_results(
        parsed.suff, dict(result=g_results, optimized=parsed.optimized)
    )


if __name__ == "__main__":
    run()
