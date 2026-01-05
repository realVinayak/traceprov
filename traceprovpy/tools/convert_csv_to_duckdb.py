import argparse
import glob
import os
import pathlib


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
        pathlib_path = pathlib.Path(path)
        table_name = pathlib_path.stem
        assert table_name not in tables_seen, f"Handling table: {table_name} again!"
        print("converting path at: ", path, " to ", table_name)
        duckdb_sql = f"/tmp/dump_{table_name}.sql"
        with open(duckdb_sql, "w") as f:
            f.write(
                f'create table {table_name} as (select * from read_csv("{path}", header = {header})) ;'
            )
        duckdb_cmd = f"duckdb {parsed.output} -f {duckdb_sql}"
        os.system(duckdb_cmd)


if __name__ == "__main__":
    main()
