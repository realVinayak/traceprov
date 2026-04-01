# creates the dataset for Polynomial Benchmark.
# It is stable, so can be done multiple times.


import argparse
import glob
from typing import Set

from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.run_with_timeout import ConnectionParams

zipf_tables = [
    "skew_1_0_num_1000",
    "skew_1_0_num_5000",
    "skew_1_0_num_10000",
    "skew_1_0_num_50000",
    "skew_1_0_num_100000",
    "skew_1_0_num_500000",
    "skew_1_0_num_1000000",
    "skew_1_0_num_5000000",
]


def add_to_many_join_data(connection):
    cursor = connection.cursor()
    control_table = "polynomial_table_control"
    cursor.execute(f"drop table if exists {control_table} cascade")
    cursor.execute(f"create table {control_table} (id int primary key, b int);")
    control_insert_table = f"insert into {control_table} (select generate_series, 1 from generate_series(0, 999))"
    cursor.execute(control_insert_table)
    global zipf_tables

    for table in zipf_tables:
        cursor.execute(
            f"alter table {table} add foreign key (z) references {control_table}(id);"
        )
        cursor.execute(f"create index if not exists idx_{table}_z ON {table} (z);")
    cursor.close()
    connection.commit()
    return set(zipf_tables) | {control_table}


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


def prepare_for_provsql(connection, tables, iters):
    global zipf_tables
    cursor = connection.cursor()
    cursor.execute("create extension provsql cascade")
    cursor.execute("SET search_path TO provsql_test,provsql,public;")
    for table in tables:
        cursor.execute(f"select add_provenance('{table}')")
        cursor.execute(
            f"select create_provenance_mapping('{table}_id', '{table}', 'id')"
        )
        connection.commit()

    connection.commit()
    factor_mapping_table = " UNION ALL ".join(
        [f"select * from polynomial_table_{idx}_id" for idx in range(iters)]
    )
    mappings = (
        f"create table factor_mapping as (select * from ({factor_mapping_table}))"
    )
    cursor.execute(mappings)
    zipf_tables_mapped = [
        f"create table zipf_factor_mapping_{f} as ( select * from {f}_id UNION ALL select * from polynomial_table_control_id)"
        for f in zipf_tables
    ]
    for sql in zipf_tables_mapped:
        cursor.execute(sql)
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
    connection = ConnectionParams.make_simple_connection(parsed)
    cursor = connection.cursor()
    cursor.execute("select version();")
    print(cursor.fetchall())
    cursor.close()

    t1 = make_data(connection, parsed.incr, parsed.num_tables)
    t2 = add_to_many_join_data(connection)
    if parsed.provsql:
        prepare_for_provsql(connection, t1 | t2, parsed.num_tables)
    connection.close()


if __name__ == "__main__":
    main()
