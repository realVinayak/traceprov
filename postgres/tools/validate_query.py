# A simple script that recursively goes through all dirs, and validates the SQL.
# It runs the queries against input database (under explain), to verify query syntax is correct.
# Supports custom tests (like, for example, if file name containing "materialization" contains "CREATE TEMP TABLE *")

import argparse
from typing import List, NamedTuple
import os
import psycopg2


class Options(NamedTuple):
    top_dir: str
    database: str
    skip: List[str]
    user: str
    password: str
    host: str
    port: str


class AbstractCheck(Exception):
    
    @classmethod
    def check(cls, connection, file_content: str, file_name: str): ...
    
    @classmethod
    def pass_(cls, file_name):
        print(cls.__name__, '(passed', file_name)

class OnlyOneStmt(AbstractCheck):

    @classmethod
    def check(cls, _, file_content: str, file_name: str):
        if file_content.count(";") > 1:
            raise cls(file_name)

        cls.pass_(file_name)

class NoInternalComment(AbstractCheck):

    @classmethod
    def check(cls, _, file_content: str, file_name: str):
        if "--" in file_content:
            raise cls(file_name)

        cls.pass_(file_name)

class ValidSchema(AbstractCheck):

    @classmethod
    def check(cls, connection, file_content: str, file_name: str):
        file_as_stmt = f'EXPLAIN {file_content.replace("\n", " ")}'
        
        try:
            cursor = connection.cursor()
            cursor.execute(file_as_stmt)
            cursor.close()
        except Exception as e:
            msg = str(e)
            raise cls(f"Failed at {file_name}: {msg}")
        
        cls.pass_(file_name)

checks: List[AbstractCheck] = [OnlyOneStmt, NoInternalComment, ValidSchema]


def validate_sql(connection, file_dir, file_name):
    abs_file_path = f"{file_name}"

    with open(abs_file_path) as f:
        sql_stmts = f.readlines()

    non_comment_stmts = [stmt for stmt in sql_stmts if not stmt.startswith("--")]
    flattend = "\n".join(non_comment_stmts)

    for check in checks:
        check.check(connection, flattend, file_name)


def recursive_check(connection, current_dir, skip_list=[]):

    for root, dirs, files in os.walk(current_dir):
        for file in files:
            complete_path = os.path.join(root, file)
            if file.endswith(".sql") and not any(to_skip in file for to_skip in skip_list):
                validate_sql(connection, root, complete_path)

        for next_dir in dirs:
            next_path = os.path.join(root, next_dir)
            recursive_check(connection, next_path, skip_list)


def main():
    parser = argparse.ArgumentParser("validate-query")
    parser.add_argument("-d", "--top_dir", required=True, type=str)
    parser.add_argument("-db", "--database", required=True)
    parser.add_argument("-x", "--skip", required=False, nargs="*")

    parser.add_argument("-u", "--user", required=False, default="postgres")
    parser.add_argument("-p", "--password", required=False, default="postgres")
    parser.add_argument("-H", "--host", required=False, default="127.0.0.1")
    parser.add_argument("-P", "--port", required=False, default="5432")

    options: Options = parser.parse_args()
    print(options)

    connection = psycopg2.connect(
        database=options.database,
        host=options.host,
        user=options.user,
        password=options.password,
        port=options.port,
    )

    recursive_check(connection, options.top_dir, options.skip)


if __name__ == "__main__":
    main()
