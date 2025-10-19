# Runs a query (from a file, or from command line, with timeout) on postgres
# Returns the time took (via EXPLAIN ANALYZE)
# Here, we also do the repeated runs (+ throwaways)

from typing import Any, Literal, NamedTuple
import psycopg2
import os
import argparse
from pathlib import PosixPath

from traceprovpy.tools.validate_query import validate_sql

DEFAULT_REPEAT = 10
DEFAULT_THROWAWAY = 5
DEFAULT_TIMEOUT = 600
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = "5432"
DEFAULT_DRY_RUN = False

CACHED_CONNECTION = "_cached_connection"


class RunParams(NamedTuple):
    repeat: None | int = DEFAULT_REPEAT
    throwaway: None | int = DEFAULT_THROWAWAY
    # the default timeout is of 10 minutes (pretty generous)
    timeout: int = DEFAULT_TIMEOUT
    # If it is dry run, don't run the actual test, but just make sure the queries
    # confirm to format correctly.
    dry_run: bool = False
    execution_time: int = None

    def validate(new_params):
        if new_params.execution_time is not None and (
            new_params.repeat is not None or new_params.throwaway is not None
        ):
            raise Exception("execution time or runtime params should be defined")
        return new_params


class ConnectionParams(NamedTuple):
    host: str
    port: str
    user: str
    password: str
    database: str

    def get_flat(self):
        flat_options = [
            cell
            for pack in dict(
                U=self.user, h=self.host, p=self.port, d=self.database
            ).items()
            for cell in [f"-{pack[0]}", pack[1]]
        ]
        return " ".join(flat_options)

    @staticmethod
    def make_simple_connection(parsed):
        connection_params = ConnectionParams(
            host=parsed.host,
            port=parsed.port,
            user=parsed.user,
            password=parsed.password,
            database=parsed.db,
        )
        return psycopg2.connect(
            database=connection_params.database,
            host=connection_params.host,
            user=connection_params.user,
            password=connection_params.password,
            port=connection_params.port,
        )


class Preprocessor:
    def preprocess(self, in_content: str) -> str:
        raise NotImplementedError("Needs to be implemented by a preprocessor")


class ReplaceFILE(Preprocessor):
    def __init__(
        self,
        replace_with_token: (
            Literal["traceprov_path", "traceprov_infer_set_path"] | PosixPath
        ),
    ):
        self.replace_with_token = replace_with_token

    def preprocess(self, in_content: str) -> str:
        if self.replace_with_token is None:
            raise Exception("Expected replace token to be filled!")
        return in_content.replace("__FILE__", self.replace_with_token)

    def __hash__(self):
        return hash((self.__class__.__name__, self.replace_with_token))

    def __repr__(self):
        return f'ReplaceFILE("{self.replace_with_token}")'


class ReplaceSelectivity(Preprocessor):
    def __init__(self, selectivity: Any):
        self.selectivity = str(selectivity)

    def preprocess(self, in_content: str) -> str:
        return in_content.replace(":selectivity", self.selectivity)

    def __hash__(self):
        return hash((self.__class__.__name__, self.selectivity))

    def __repr__(self):
        return f"ReplaceSelectivity('{self.selectivity}')"


class RunWithTimeoutOptions(NamedTuple):
    file_path: str
    connection_params: ConnectionParams
    capture_output: bool = False
    params: RunParams = RunParams()
    # Just some extra context stuff (like connections)
    extras: dict | None = None
    skip_validation: bool = False
    preprocessors: list[Preprocessor] = []
    strict_run: bool = False

    def close_all(self):
        if self.extras is None:
            return
        cached_connection = self.extras.get(CACHED_CONNECTION)
        cursor = cached_connection.cursor()
        cursor.execute("COMMIT;")
        cursor.close()
        if cached_connection:
            cached_connection.close()

    def get_explain(self, connection):
        if connection.server_version >= 180000:
            return "EXPLAIN (analyze, timing off, buffers off, memory off, format JSON)"
        else:
            return "EXPLAIN (analyze, timing off, buffers off, format JSON)"


def run_with_timeout(options: RunWithTimeoutOptions) -> float | None | dict:

    file_dir = os.path.dirname(options.file_path)

    # The caching is used just once.
    # That is, if the extras is a dict, then connection is stored.

    cached_connection = None
    should_cache_connection = False
    if options.extras is not None:
        should_cache_connection = True
        cached_connection = options.extras.get(CACHED_CONNECTION)

    connection = cached_connection or psycopg2.connect(
        database=options.connection_params.database,
        host=options.connection_params.host,
        user=options.connection_params.user,
        password=options.connection_params.password,
        port=options.connection_params.port,
    )

    if (
        options.extras is not None
        and should_cache_connection
        and cached_connection is None
    ):
        options.extras[CACHED_CONNECTION] = connection

    # Don't bother verifying, for now....
    if not options.skip_validation and len(options.preprocessors) == 0:
        validate_sql(connection, file_dir, options.file_path)

    if options.params.dry_run:
        return -1

    # The only comments that we've are external. So, first get rid of initial lines with comments.
    with open(options.file_path) as f:
        sql_stmts = f.readlines()
        non_comment_stmts = [stmt for stmt in sql_stmts if not stmt.startswith("--")]
        flattend_sql_query = " ".join(non_comment_stmts)

    for preprocessor in options.preprocessors:

        flattend_sql_query = preprocessor.preprocess(flattend_sql_query)

    timeout_stmt = f"SET statement_timeout = '{options.params.timeout}s';"
    augmented_sql = f"{options.get_explain(connection)} {flattend_sql_query}"

    cursor = connection.cursor()
    try:
        if not options.strict_run:
            cursor.execute(timeout_stmt)
            cursor.execute(augmented_sql)
            analyze_result = cursor.fetchall()[0][0][0]
            planning_time = analyze_result["Planning Time"]
            execution_time = analyze_result["Execution Time"]
            computed_time = dict(
                explain_time=float((planning_time + execution_time) / 1000)
            )

        if options.capture_output or options.strict_run:
            computed_time = None
            # Now, need to run the query again.
            cursor.execute(flattend_sql_query)
            try:
                captured_result = cursor.fetchall()
            except psycopg2.ProgrammingError as e:
                if "no results to fetch" in str(e):
                    captured_result = None
                else:
                    raise e
            new_result = dict(timing=computed_time, captured=captured_result)
            computed_time = new_result
    except psycopg2.errors.QueryCanceled:
        computed_time = None

    cursor.close()
    # Don't close if caching the connection.
    if not should_cache_connection:
        connection.close()

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
    parser.add_argument(
        "-c",
        "--capture",
        required=False,
        action=argparse.BooleanOptionalAction,
        default=False,
    )

    parsed = parser.parse_args()
    run_options = RunParams(
        repeat=parsed.repeat,
        throwaway=parsed.throwaway,
        dry_run=parsed.dry_run,
        timeout=parsed.timeout,
    )

    connection_params = ConnectionParams(
        host=parsed.host,
        port=parsed.port,
        user=parsed.user,
        password=parsed.password,
        database=parsed.database,
    )
    runtime_options = RunWithTimeoutOptions(
        file_path=parsed.file_path,
        params=run_options,
        capture_output=parsed.capture,
        connection_params=connection_params,
    )
    measured_time = run_with_timeout(runtime_options)
    print("measured time: ", measured_time)


if __name__ == "__main__":
    run_from_cmd()
