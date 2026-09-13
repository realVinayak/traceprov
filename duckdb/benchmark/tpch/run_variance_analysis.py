import argparse
import os
from pathlib import Path
import re

from traceprovpy.tools.extract_query_results import (
    dump_result,
    dump_rows_list,
    handle_duckdb,
)

DUCKDB_RE = r"local_test_variance_duckdb_run_no_use_table_def-y__optimized-y__threads-\d+_(\d+)_2026"
TRACEPROV_RE = r"local_test_variance_traceprov_run_optimized-y__threads-\d+_(\d+)_2026"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--sf", required=True)

    parsed = parser.parse_args()
    traceprov_results = []
    duckdb_results = []
    for root, _, files in os.walk(parsed.dir):
        for file in files:
            complete_path = os.path.join(root, file)
            if re.findall(DUCKDB_RE, complete_path):
                duckdb_results.append(complete_path)
            elif re.findall(TRACEPROV_RE, complete_path):
                traceprov_results.append(complete_path)

    traceprov_results = list(sorted(traceprov_results))
    duckdb_results = list(sorted(duckdb_results))

    print("TraceProv Results: ", traceprov_results)
    print("DuckDB Results: ", duckdb_results)

    print("TraceProv Result Count: ", len(traceprov_results))
    print("DuckDB Result Count: ", len(duckdb_results))

    all_rows_list = []
    all_duckdb_rows_list = []
    version_counter = 1
    for file in traceprov_results:
        version_counter += 1
        handle_duckdb("traceprov", "all", Path(file), all_rows_list, version_counter)

    for file in duckdb_results:
        version_counter += 1
        handle_duckdb(
            "traceprov", "all", Path(file), all_duckdb_rows_list, version_counter
        )

    traceprov_dir = Path(parsed.out_dir) / "traceprov"
    duckdb_dir = Path(parsed.out_dir) / "duckdb"
    os.makedirs(traceprov_dir, exist_ok=True)
    os.makedirs(duckdb_dir, exist_ok=True)
    dump_rows_list(parsed.sf, "duckdb", "all", all_rows_list, traceprov_dir)
    dump_rows_list(parsed.sf, "duckdb", "all", all_duckdb_rows_list, duckdb_dir)


if __name__ == "__main__":
    main()
