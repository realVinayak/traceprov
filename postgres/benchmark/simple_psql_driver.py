import psycopg2
import argparse
import time
import json

def main():
    parser = argparse.ArgumentParser(prog="simple-psql-driver")
    parser.add_argument("-U", "--user", required=True)
    parser.add_argument("-d", "--db", required=True)
    parser.add_argument("-p", "--password", required=True)
    parser.add_argument("-f", "--file", required=True)

    options = parser.parse_args()
    connection = psycopg2.connect(
        database=options.db,
        host="127.0.0.1",
        user=options.user,
        password=options.password,
        port="5432",
    )

    with open(options.file) as f:
        sql_stmts = f.read().replace("\n", " ")

    assert sql_stmts.count(";") == 1

    cursor = connection.cursor()

    begin_execute = time.perf_counter()
    cursor.execute(f"EXPLAIN (analyze, timing off, format JSON) {sql_stmts}")
    end_execute = time.perf_counter()
    try:
        result = (list(cursor.fetchall())[0][0][0]['Execution Time'])
        print(result)
    except:
        pass
    end_fetch = time.perf_counter()

    execution_time = end_execute - begin_execute
    fetch_time = end_fetch - end_execute

    print(f"execution: {execution_time}; fetch: {fetch_time}")


if __name__ == "__main__":
    main()
