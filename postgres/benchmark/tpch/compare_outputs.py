import argparse
import json

import os

special_selects = {
    7: "s_suppkey,l_orderkey,l_linenumber,o_orderkey,c_custkey,n1_nationkey,n2_nationkey"
}
# COPY (SELECT * FROM read_csv('layer_0.csv', header=true) order by all) to "test.csv" (header false);


def dump_normalized(
    path: str,
    has_header: bool,
    out_path: str,
    ignore_column_count: int = 0,
    special_selects: str = "",
):
    header_value = "true" if has_header else "false"
    read_csv_function = f"read_csv({path}, header={header_value})"
    if ignore_column_count:
        # ugh
        exclude_columns = ",".join([f"column_{i}" for i in range(ignore_column_count)])
        exclude = f"exclude({exclude_columns})"
    else:
        exclude = ""
    select_column = "*" if special_selects == "" else special_selects
    select_expr = (
        f"select {select_column} {exclude} from {read_csv_function} order by all"
    )
    dump_expr = f'COPY ({select_expr}) TO "{out_path}" (header false)'
    with open("/tmp/duckdb_query.sql", "w") as f:
        f.write(dump_expr)
    print(dump_expr)
    assert os.system(f'duckdb -f "/tmp/duckdb_query.sql"') == 0
    with open("/tmp/duckdb_query_count.sql", "w") as f:
        f.write(
            f'COPY (select count(*) from "{out_path}") to "/tmp/count.out" (header false)'
        )
    assert os.system(f'duckdb -f "/tmp/duckdb_query_count.sql"') == 0
    with open("/tmp/count.out") as f:
        count = int(f.read())
    return count


def compare_outputs(
    base: str, new: str, subdir: str, query: int, base_name: str, new_name: str
):
    print("on: ", subdir, query)
    base_path = f'"{base}/{subdir}/{query}/{base_name}"'
    new_path = f'"{new}/{subdir}/{query}/{new_name}"'
    base_out_path = f"{subdir}_{query}_base.csv"
    new_out_path = f"{subdir}_{query}_new.csv"
    count_base = dump_normalized(
        base_path, True, base_out_path, 0, special_selects.get(query, "")
    )
    count_new = dump_normalized(new_path, True, new_out_path, 1)
    assert os.path.exists(base_out_path) and os.path.exists(new_out_path)
    print("comparing: ", base_out_path, new_out_path)
    assert os.system(f"diff {base_out_path} {new_out_path} > temp.out") == 0
    assert count_base == count_new
    print("compared counts: ", count_base, count_new)


def main():
    parser = argparse.ArgumentParser(prog="compare-outputs")
    parser.add_argument("-b", "--base", required=True, type=str)
    parser.add_argument("-bn", "--base_name", required=True, type=str)
    parser.add_argument("-n", "--new", required=True, type=str)
    parser.add_argument("-nn", "--new_name", required=True, type=str)
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parsed = parser.parse_args()

    with open(parsed.config) as f:
        config = json.loads(f.read())

    for subdir in config["subdirs"]:
        for query in config["queries"]:
            compare_outputs(
                parsed.base,
                parsed.new,
                subdir,
                query,
                parsed.base_name,
                parsed.new_name,
            )


if __name__ == "__main__":
    main()
