# A simple script that recursively goes through all dirs, and validates the SQL.
# It runs the queries against input database (under explain), to verify query syntax is correct.
# Supports custom tests (like, for example, if file name containing "materialization" contains "CREATE TEMP TABLE *")

import argparse
from typing import List, NamedTuple
import os
import psycopg2


class Options(NamedTuple):
    top_dir: str
    db: str
    skip: List[str]
    user: str
    password: str
    host: str
    port: str


class AbstractCheck(NamedTuple):

    def check(connection, file_content: str): ...


class OnlyOneStmt(AbstractCheck):

    exception = Exception("Only one stmt, found more than semicolons!")

    @staticmethod
    def check(_, file_content: str):
        if file_content.count(":") > 1:
            raise OnlyOneStmt.exception


class NoInternalComment(AbstractCheck):

    exception = Exception("No internal comments allowed")

    @staticmethod
    def check(_, file_content: str):
        if "--" in file_content:
            raise NoInternalComment.exception


class ValidSchema(AbstractCheck):

    @staticmethod
    def check(connection, file_content: str):
        file_as_stmt = f'EXPLAIN {file_content.replace("\n", " ")}'
        cursor = connection.cursor()
        cursor.execute(file_as_stmt)
        cursor.close()


checks: List[AbstractCheck] = [OnlyOneStmt, NoInternalComment, ValidSchema]


def validate_sql(connection, file_dir, file_name):
    abs_file_path = f"{file_dir}/{file_name}"

    with open(abs_file_path) as f:
        sql_stmts = f.readline()

    non_comment_stmts = [stmt for stmt in sql_stmts if not stmt.startswith("--")]
    flattend = "\n".join(non_comment_stmts)

    for check in checks:
        check.check(connection, flattend)


def recursive_check(connection, current_dir, skip_list=[]):

    for root, dirs, files in os.walk(current_dir):
        for file in files:
            if file.endswith(".sql") and file not in skip_list:
                validate_sql(connection, root, file)

        for next_dir in dirs:
            next_path = os.path.join(root, next_dir)
            recursive_check(connection, next_path)


def main():
    parser = argparse.ArgumentParser("validate-query")
    parser.add_argument("-d", "--top_dir", required=True, type=str)
    parser.add_argument("-db", "--database", required=True)
    parser.add_argument("-x", "--skip", required=False, nargs="*")

    parser.add_argument("-u", "--user", required=False, default="postgres")
    parser.add_argument("-p", "--password", required=False, default="postgres")
    parser.add_argument("-h", "--host", required=False, default="127.0.0.1")
    parser.add_argument("-P", "--port", required=False, default="5432")

    options: Options = parser.parse_args()

    connection = psycopg2.connect(
        database=options.db,
        host=options.host,
        user=options.user,
        password=options.password,
        port=options.port,
    )

    recursive_check(connection, options.top_dir, options.skip)


if __name__ == "__main__":
    main()
