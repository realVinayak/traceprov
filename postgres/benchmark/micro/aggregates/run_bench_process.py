import argparse
import os
from typing import NamedTuple
import time
import uuid
import psycopg2


class Options(NamedTuple):
    file: str
    q: str
    outfile: str
    db: str
    timeout: int
    driver: bool
    analyze: bool


def resolve_filename(file_name: str, query_num: str):
    # We've some
    if file_name == "AUTO_TRACEPROV_TIME":
        return f"templates/traceprov/{query_num}.infertime.sql"

    if file_name == "AUTO_TRACEPROV_MAT":
        return f"templates/traceprov/{query_num}.materialize.sql"

    return file_name


def main():
    parser = argparse.ArgumentParser(prog="aggregation-microbench-proc")
    parser.add_argument("-f", "--file", required=True, type=str)
    parser.add_argument("-q", "--q", required=True, type=str)
    parser.add_argument("-out", "--outfile", required=True, type=str)
    parser.add_argument("-db", "--db", required=True, type=str)
    parser.add_argument("-t", "--timeout", required=True, type=int)
    parser.add_argument(
        "--driver", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--analyze", action=argparse.BooleanOptionalAction, default=False
    )

    parsed: Options = parser.parse_args()
    filename = resolve_filename(parsed.file, parsed.q)

    connection = psycopg2.connect(
        database=parsed.db,
        host="127.0.0.1",
        user="postgres",
        password="postgres",
        port="5432",
    )

    is_time = parsed.file == "AUTO_TRACEPROV_TIME"

    new_file_name = f"/tmp/{uuid.uuid4()}_file.sql"

    with open(filename) as f:
        new_sql = f"SET statement_timeout='{parsed.timeout}s';\n" + f.read()

    with open(new_file_name, "w") as wf:
        wf.write(new_sql)

    if is_time:
        os.system(
            f"PGPASSWORD=postgres psql -U postgres {parsed.db} -f {new_file_name} --tuples > /tmp/time.out"
        )
        with open("/tmp/time.out") as f:
            res_ = f.read().replace("SET", "")
            computed_time = int(res_.strip().replace("\n", "")) / 1000
    else:
        if parsed.driver:
            print("using driver")
            cursor = connection.cursor()
            start = time.perf_counter()
            cursor.execute(new_sql)
            end = time.perf_counter()
            computed_time = end - start
        elif parsed.analyze:
            print("using analyze")
            cleaned_sql = new_sql.replace("\n", " ")
            assert cleaned_sql.count(";") == 1
            augmented_sql = f"EXPLAIN (analyze, timing off, format JSON) {cleaned_sql}"
            cursor = connection.cursor()
            cursor.execute(augmented_sql)
            _analyze_result = cursor.fetchall()[0][0][0]
            print(_analyze_result)
            planning_time = _analyze_result["Plan"]["Planning Time"]
            execution_time = _analyze_result["Execution Time"]
            computed_time = (planning_time + execution_time) / 1000
        else:
            print("using shell")
            start = time.perf_counter()
            os.system(
                f"PGPASSWORD=postgres psql -U postgres {parsed.db} -f {new_file_name} --tuples > /dev/null"
            )
            end = time.perf_counter()
            computed_time = end - start

    with open(parsed.outfile, "w") as f:
        print("time: ", computed_time)
        f.write(str(computed_time))


if __name__ == "__main__":
    main()
