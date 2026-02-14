import argparse
import glob
import os
import pathlib

from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run


def convert_csv_to_duckdb(input_path: str, output_path: str, header: bool):
    pathlib_path = pathlib.Path(input_path)
    table_name = pathlib_path.stem
    print("converting path at: ", input_path, " to ", table_name)
    duckdb_sql = f"/tmp/dump_{table_name}.sql"
    with open(duckdb_sql, "w") as f:
        f.write(
            f"create table {table_name} as (select * from read_csv(\"{input_path}\", header = {header}, auto_type_candidates = ['BIGINT'])) ;"
        )
    duckdb_cmd = f"cat {duckdb_sql} | duckdb {output_path}"
    traceprov_assert_safe_run(duckdb_cmd)
    return table_name


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", required=True)
    parser.add_argument("-o", "--output", required=True)
    parser.add_argument(
        "--header", action=argparse.BooleanOptionalAction, default=False
    )

    parsed = parser.parse_args()
    os.system(f"rm -f {parsed.output}")
    header = "true" if parsed.header else "false"
    paths = glob.glob(parsed.input)
    tables_seen = set()
    for path in paths:
        assert ".csv" in path
        assert path not in tables_seen, f"Handing {path} again!"
        tables_seen.add(path)
        convert_csv_to_duckdb(path, parsed.output, header)


if __name__ == "__main__":
    main()
