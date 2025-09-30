# Runs a query (from a file, or from command line, with timeout) on postgres
# Returns the time took (via EXPLAIN ANALYZE)
# Here, we also do the repeated runs (+ throwaways)

from typing import NamedTuple
import psycopg2
import os
import argparse

from .validate_query import validate_sql

DEFAULT_REPEAT = 10
DEFAULT_THROWAWAY = 5
DEFAULT_TIMEOUT = 600
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = "5432"
DEFAULT_DRY_RUN = False


class RunWithTimeoutOptions(NamedTuple):
    user: str
    password: str
    db: str
    file_path: str
    repeat: int = DEFAULT_REPEAT
    throwaway: int = DEFAULT_THROWAWAY
    # the default timeout is of 10 minutes (pretty generous)
    timeout: int = DEFAULT_TIMEOUT
    host: str = DEFAULT_HOST
    port: str = DEFAULT_PORT
    # If it is dry run, don't run the actual test, but just make sure the queries
    # confirm to format correctly.
    dry_run: bool = False


def run_with_timeout(options: RunWithTimeoutOptions) -> float | None:

    file_dir = os.path.dirname(options.file_path)

    connection = psycopg2.connect(
        database=options.db,
        host=options.host,
        user=options.user,
        password=options.password,
        port=options.port,
    )

    validate_sql(connection, file_dir, options.file_path)

    if options.dry_run:
        return -1

    # The only comments that we've are external. So, first get rid of initial lines with comments.
    with open(options.file_path) as f:
        sql_stmts = f.readlines()
        non_comment_stmts = [stmt for stmt in sql_stmts if not stmt.startswith("--")]
        flattend_sql_query = " ".join(non_comment_stmts)

    timeout_stmt = f"SET statement_timeout = '{options.timeout}s';"
    augmented_sql = f"EXPLAIN (analyze, timing off, format JSON) {flattend_sql_query}"

    cursor = connection.cursor()
    try:
        cursor.execute(timeout_stmt)
        cursor.execute(augmented_sql)
        analyze_result = cursor.fetchall()[0][0][0]
        # print(analyze_result)
        planning_time = analyze_result["Planning Time"]
        execution_time = analyze_result["Execution Time"]
        computed_time = float((planning_time + execution_time) / 1000)
    except psycopg2.errors.QueryCanceled:
        computed_time = None

    cursor.close()

    return computed_time


def run_from_cmd():
    parser = argparse.ArgumentParser(prog="run-with-timeout")
    parser.add_argument("-u", "--user", required=True)
    parser.add_argument("-p", "--password", required=True)
    parser.add_argument("-db", "--database", required=True)
    parser.add_argument("-f", "--file_path", required=True)
    parser.add_argument(
        "-r", "--repeat", required=False, default=DEFAULT_REPEAT, type=int
    )
    parser.add_argument(
        "-thway",
        "--throwaway",
        required=False,
        default=DEFAULT_THROWAWAY,
        type=int,
    )
    parser.add_argument("-H", "--host", required=False, default=DEFAULT_HOST)
    parser.add_argument("-P", "--port", required=False, default=DEFAULT_PORT)
    parser.add_argument(
        "--dry_run",
        action=argparse.BooleanOptionalAction,
        default=DEFAULT_DRY_RUN,
    )
    parser.add_argument("-t", "--timeout", required=False, default=DEFAULT_TIMEOUT)

    parsed = parser.parse_args()
    runtime_options = RunWithTimeoutOptions(
        user=parsed.user,
        password=parsed.password,
        db=parsed.database,
        file_path=parsed.file_path,
        repeat=parsed.repeat,
        throwaway=parsed.throwaway,
        host=parsed.host,
        port=parsed.port,
        dry_run=parsed.dry_run,
        timeout=parsed.timeout,
    )
    measured_time = run_with_timeout(runtime_options)
    print("measured time: ", measured_time)


if __name__ == "__main__":
    run_from_cmd()
