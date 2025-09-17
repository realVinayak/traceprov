import argparse
from typing import NamedTuple
from numpy import random
import time
import psycopg2
import os


class Options(NamedTuple):
    skew: float
    num: int
    db: str
    drop: bool


# create a max of 5000 at a time for memory reasons.
CHUNK_SIZE = 5000

VALUE_MIN = 1
VALUE_MAX = 100


def insert_value(z_value, value, cursor, table):
    cursor.execute(f"INSERT INTO {table} (z, val) VALUES ({z_value}, {value})")


def main():
    parser = argparse.ArgumentParser(prog="prepare-aggregate-data")
    parser.add_argument("-skew", "--skew", required=True, type=float)
    parser.add_argument("-num", "--num", required=True, type=int)
    parser.add_argument("-db", "--db", required=True, type=str)
    parser.add_argument("--drop", action=argparse.BooleanOptionalAction, default=False)

    # parser.add_argument("-db", "--db", required=True, type=str)
    parsed: Options = parser.parse_args()

    pg_passwd = os.getenv("PGPASSWORD")
    assert pg_passwd
    base_connection = psycopg2.connect(
        database="postgres",
        host="127.0.0.1",
        user="postgres",
        password=pg_passwd,
        port="5432",
    )

    if parsed.drop:
        cursor = base_connection.cursor()
        cursor.execute(f"COMMIT;")
        cursor.execute(f"DROP DATABASE IF EXISTS {parsed.db};")
        cursor.execute(f"CREATE DATABASE {parsed.db};")

    db_connection = psycopg2.connect(
        database=parsed.db,
        host="127.0.0.1",
        user="postgres",
        password=pg_passwd,
        port="5432",
    )
    db_cursor = db_connection.cursor()
    db_cursor.execute("select 1;")
    print(db_cursor.fetchall())

    table_name = f"skew_{parsed.skew}_num_{parsed.num}".replace(".", "_")
    print("storing in", table_name)
    db_cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
    db_cursor.execute(
        f"CREATE TABLE {table_name} (z INTEGER NOT NULL, val INTEGER NOT NULL);"
    )
    print("create table;")

    remaining = parsed.num

    while remaining != 0:
        create_count = min(CHUNK_SIZE, remaining)
        remaining -= create_count
        assert create_count >= 0
        distribution = random.zipf(parsed.skew, create_count)
        for row_index in range(create_count):
            z_value = distribution[row_index]
            val_value = random.randint(VALUE_MIN, VALUE_MAX)
            insert_value(z_value, val_value, db_cursor, table_name)
    db_cursor.execute(f"COMMIT;")


if __name__ == "__main__":
    main()
