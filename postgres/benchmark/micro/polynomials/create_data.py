# creates the dataset for Polynomial Benchmark.
# It is stable, so can be done multiple times.


import argparse
import glob
from typing import Set

from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.make_zipf_data import (
    ZipfOptions,
    make_data as make_zipf_data_func,
)
from traceprovpy.tools.run_with_timeout import ConnectionParams
from itertools import product

zipf_tables = [
    "skew_1_0_num_1000",
    "skew_1_0_num_5000",
    # "skew_1_0_num_10000",
    # "skew_1_0_num_50000",
    # "skew_1_0_num_100000",
    # "skew_1_0_num_500000",
    # "skew_1_0_num_1000000",
    # "skew_1_0_num_5000000",
]

GROUP_COUNT = 1000


def get_table_nums():
    num_rows = [int(table.split("_")[-1]) for table in zipf_tables]
    return num_rows


def get_control_table(table_id):
    return f"polynomial_table_control_{table_id}"


def add_to_many_join_data(connection, control_table_count):
    cursor = connection.cursor()
    control_tables = []
    for control_table_id in range(control_table_count):
        control_table = get_control_table(control_table_id)
        cursor.execute(f"drop table if exists {control_table} cascade")
        cursor.execute(f"create table {control_table} (id int primary key, b int);")
        control_insert_table = f"insert into {control_table} (select generate_series, 1 from generate_series(0, {GROUP_COUNT-1}))"
        cursor.execute(control_insert_table)
        control_tables.append(control_table)
    global zipf_tables
    for table, control_table in product(zipf_tables, control_tables):
        cursor.execute(
            f"alter table {table} add foreign key (z) references {control_table}(id);"
        )
        cursor.execute(
            f"create index if not exists idx_{table}_{control_table}_z ON {table} (z);"
        )
    cursor.close()
    connection.commit()
    return set(zipf_tables) | set(control_tables)


def make_data(connection, incr, num_tables):
    tables = set()
    cursor = connection.cursor()
    for table_idx in range(num_tables):
        table_name = f"polynomial_table_{table_idx}"
        cursor.execute(f"drop table if exists {table_name}")
        create_sql = f"create table {table_name} (id int primary key, b int);"
        cursor.execute(create_sql)
        insert_sql = f"insert into {table_name} (select generate_series, 1 from generate_series (1, 1 << {incr}))"
        cursor.execute(insert_sql)
        tables.add(table_name)
    cursor.close()
    connection.commit()
    return tables


def prepare_for_provsql(connection, tables, factor_iters, join_iters):
    global zipf_tables
    cursor = connection.cursor()
    cursor.execute("create extension if not exists provsql cascade")
    cursor.execute("SET search_path TO provsql_test,provsql,public;")
    for table in tables:
        cursor.execute(f"select add_provenance('{table}')")
        cursor.execute(
            f"select create_provenance_mapping('{table}_id', '{table}', 'id')"
        )
        connection.commit()

    connection.commit()
    factor_mapping_table = " UNION ALL ".join(
        [f"select * from polynomial_table_{idx}_id" for idx in range(factor_iters)]
    )
    mappings = (
        f"create table factor_mapping as (select * from ({factor_mapping_table}))"
    )
    cursor.execute(mappings)
    for f in zipf_tables:
        for j_count in range(1, join_iters + 1):
            control_tables = [
                f"{get_control_table(ct_id)}_id" for ct_id in range(j_count)
            ]
            all_tables = [f"{f}_id", *control_tables]
            unioned = " UNION ALL ".join(
                [f"select * from {table}" for table in all_tables]
            )
            table_creation = (
                f"create table zipf_factor_mapping_{f}_{j_count} as ({unioned})"
            )
            cursor.execute(table_creation)
            connection.commit()


def main():
    parser = argparse.ArgumentParser(prog="polynomial-make-data")
    parser.add_argument("-incr", "--incr", required=True, type=int)
    parser.add_argument("-table", "--num_tables", required=True, type=int)
    parser.add_argument(
        "--provsql", action=argparse.BooleanOptionalAction, default=False
    )
    postgres_connection_from_cmd(parser)
    parsed = parser.parse_args()

    connection_params = ConnectionParams.make_from_parsed(parsed)
    table_nums = get_table_nums()
    for table_num in table_nums:
        zipf_options = ZipfOptions(
            skew=1.0, num=table_num, db=parsed.db, drop=False, num_groups=1000
        )
        make_zipf_data_func(zipf_options, connection_params)

    connection = ConnectionParams.make_simple_connection(parsed)
    cursor = connection.cursor()
    cursor.execute("select version();")
    print(cursor.fetchall())
    cursor.close()

    t1 = make_data(connection, parsed.incr, parsed.num_tables)
    t2 = add_to_many_join_data(connection, 8)
    if parsed.provsql:
        prepare_for_provsql(connection, t1 | t2, parsed.num_tables, 8)
    connection.close()


if __name__ == "__main__":
    main()
