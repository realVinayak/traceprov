import argparse
from typing import NamedTuple
from numpy import random  # pyright: ignore[reportMissingImports]
import time
import psycopg2  # pyright: ignore[reportMissingModuleSource]
import os
import numpy as np
from functools import reduce


class Options(NamedTuple):
    skew: float
    num: int
    db: str
    drop: bool
    num_groups: int


# create a max of 5000 at a time for memory reasons.
CHUNK_SIZE = 5000

VALUE_MIN = 1
VALUE_MAX = 100


def insert_value(z_value, value, cursor, table):
    values = [f"({_z_val}, {_val})" for (_z_val, _val) in zip(z_value, value)]
    cursor.execute(f"INSERT INTO {table} (z, val) VALUES {','.join(values)};")


def generate_zifpian_distribution(num_groups, skew_param):
    zeta_value = sum([1 / pow(i, skew_param) for i in range(1, num_groups + 1)])
    probs = [1 / (pow(i, skew_param) * zeta_value) for i in range(1, num_groups + 1)]
    assert np.isclose(sum(probs), 1)
    #print(sum(probs))
    return probs


def make_cdf(probs):

    def _reducer(previous, current):
        last = previous[-1]
        return [*previous, current + last]

    return reduce(_reducer, probs[1:], [probs[0]])


def _draw(input_probs):
    random_value = np.random.random()
    found = -1
    for idx, prob in enumerate(input_probs):
        if random_value <= prob:
            found = idx
            break
    if found == -1:
        print("truncating beyond to end")
        found = len(input_probs) - 1
    return found


def draw(card, num_groups, skew):
    probs = generate_zifpian_distribution(num_groups, skew)
    input_probs = make_cdf(probs)
    return [_draw(input_probs) for _ in range(card)]


def main():
    parser = argparse.ArgumentParser(prog="prepare-aggregate-data")
    parser.add_argument("-skew", "--skew", required=True, type=float)
    parser.add_argument("-num", "--num", required=True, type=int)
    parser.add_argument("-db", "--db", required=True, type=str)
    parser.add_argument("--drop", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("-g", "--num_groups", required=True, type=int)

    # parser.add_argument("-db", "--db", required=True, type=str)
    parsed: Options = parser.parse_args()

    db_name = f"{parsed.db}_{parsed.num_groups}"
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
        cursor.execute(f"DROP DATABASE IF EXISTS {db_name};")
        cursor.execute(f"CREATE DATABASE {db_name};")

    db_connection = psycopg2.connect(
        database=db_name,
        host="127.0.0.1",
        user="postgres",
        password=pg_passwd,
        port="5432",
    )
    print('db name: ', db_name)
    db_cursor = db_connection.cursor()
    db_cursor.execute("select 1;")
    print(db_cursor.fetchall())

    table_name = f"skew_{parsed.skew}_num_{parsed.num}".replace(".", "_")
    print("storing in", table_name)
    db_cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
    db_cursor.execute(
        f"CREATE TABLE {table_name} (id SERIAL PRIMARY KEY, z BIGINT NOT NULL, val INTEGER NOT NULL);"
    )
    print("create table;")

    remaining = parsed.num

    while remaining != 0:
        create_count = min(CHUNK_SIZE, remaining)
        remaining -= create_count
        assert create_count >= 0
        # distribution = random.zipf(parsed.skew, create_count)
        distribution = draw(create_count, parsed.num_groups, parsed.skew)
        random_ints = []
        for _ in range(create_count):
            random_ints.append(random.randint(VALUE_MIN, VALUE_MAX))
        insert_value(list(distribution), random_ints, db_cursor, table_name)
    db_cursor.execute(f"COMMIT;")


if __name__ == "__main__":
    main()
