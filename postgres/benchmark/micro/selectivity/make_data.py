import argparse
from psycopg2.extras import execute_values

from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.run_with_timeout import ConnectionParams
from traceprovpy.utils import add_underscores_numbers

DB_PREFIX = "microbench_selectivity_"

CHUNK_SIZE = 10000


def insert_partition(cursor, partition_size, group_index, sql):
    assert group_index > 0
    remaining = partition_size
    while remaining != 0:
        create_count = min(CHUNK_SIZE, remaining)
        remaining -= create_count
        assert create_count >= 0
        tuples = [
            (group_index + _id, -1 * (group_index), group_index)
            for _id in range(create_count)
        ]
        execute_values(cursor, sql, tuples)


def make_data(connection, num_rows: int, num_groups: int) -> None:
    partition = num_rows / num_groups
    assert num_rows % num_groups == 0  # make things simpler.
    with open("create_table.template.sql") as f:
        create_table_template = f.read()

    replacer = lambda raw_str: raw_str.replace(
        "%NUM_ROWS%", add_underscores_numbers(num_rows)
    )
    create_table_sql = replacer(create_table_template)
    table_name = replacer(f"data_table_%NUM_ROWS%")
    sql = f"insert into {table_name} (min_value, negative_group_number, group_number) values %s"
    cursor = connection.cursor()
    cursor.execute("BEGIN;")
    try:
        print(create_table_sql)
        cursor.execute(create_table_sql)
        for i in range(1, num_groups + 1):
            print("on: ", (i), "/", num_groups)
            insert_partition(cursor, partition, i, sql)
        # raise Exception("tests")
        cursor.execute("COMMIT;")
    except:
        cursor.execute("ROLLBACK;")
        raise
    cursor.close()


def main():
    parser = argparse.ArgumentParser(prog="selectivity-make-data")
    parser.add_argument("-g", "--num_groups", type=int, required=True)
    parser.add_argument("-n", "--num_rows", required=True, type=str)
    # for testing, this is useful
    parser.add_argument("--data", action=argparse.BooleanOptionalAction, default=False)

    postgres_connection_from_cmd(parser)
    parsed = parser.parse_args()

    num_rows: str | None = None
    num_groups = parsed.num_groups
    try:
        num_rows = int(parsed.num_rows)
    except ValueError:
        raw_num_rows: str = parsed.num_rows.lower()
        assert raw_num_rows.count("m") == 1
        num_rows = int(raw_num_rows.replace("m", "")) * (1_000_000)

    assert isinstance(num_rows, int)
    print(num_rows, num_groups)
    connection = ConnectionParams.make_simple_connection(parsed)
    cursor = connection.cursor()
    cursor.execute("select version();")
    print(cursor.fetchall())
    cursor.close()
    if parsed.data:
        make_data(connection, num_rows, num_groups)
    else:
        print("info: skipping data creation")
    connection.close()


if __name__ == "__main__":
    main()
