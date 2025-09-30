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
    fix: bool


show_output = False


class AbstractCheck(Exception):

    @classmethod
    def check(cls, connection, file_content: str, file_name: str): ...

    @classmethod
    def pass_(cls, file_name):
        if show_output:
            print(cls.__name__, "(passed", file_name)

    @classmethod
    def fix(cls, connection, file_content: str, file_name: str):
        raise NotImplementedError("Not implemented for base class!")


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
        file_content_flat = file_content.replace("\n", " ")
        file_as_stmt = f"EXPLAIN {file_content_flat}"

        try:
            cursor = connection.cursor()
            cursor.execute(file_as_stmt)
            cursor.close()
        except Exception as e:
            msg = str(e)
            raise cls(f"Failed at {file_name}: {msg}")

        cls.pass_(file_name)


class MaterializationPresent(AbstractCheck):

    @classmethod
    def check(cls, connection, file_content: str, file_name: str):

        if "materialize" not in file_name:
            return

        file_content = file_content.lower()

        if not file_content.startswith("create temp table"):
            raise cls(file_name)

        cls.pass_(file_name)

    @classmethod
    def fix(cls, connection, file_content: str, file_name: str):

        print(f"(cls.__name__): Fixing - {file_name}")
        assert file_content.lower().startswith("with")
        assert file_content.count(";") == 1

        file_content = file_content.replace(";", ");")
        new_file_content = "CREATE TEMP TABLE gprom_lineage AS (" + file_content
        return new_file_content


checks: List[AbstractCheck] = [
    OnlyOneStmt,
    NoInternalComment,
    ValidSchema,
    MaterializationPresent,
]


def validate_sql(connection, file_dir, file_name, try_fix=False):
    abs_file_path = f"{file_name}"

    with open(abs_file_path) as f:
        sql_stmts = f.readlines()

    non_comment_stmts = [stmt for stmt in sql_stmts if not stmt.startswith("--")]
    flattend = "\n".join(non_comment_stmts)

    for check in checks:
        try:
            check.check(connection, flattend, file_name)
        except AbstractCheck as e:
            if try_fix:
                new_content = e.fix(connection, flattend, file_name)
                try:
                    check.check(connection, new_content, file_name)
                    with open(f"{file_name}.backup", "w") as f:
                        f.write(flattend)
                    with open(file_name, "w") as f:
                        f.write(new_content)
                    flattend = new_content
                except Exception as e:
                    print("Error fixing again!")
                    raise e
            else:
                raise e


def recursive_check(connection, current_dir, skip_list=[], try_fix=False):

    for root, dirs, files in os.walk(current_dir):
        for file in files:
            complete_path = os.path.join(root, file)
            if file.endswith(".sql") and not any(
                to_skip in file for to_skip in skip_list
            ):
                validate_sql(connection, root, complete_path, try_fix)

        for next_dir in dirs:
            next_path = os.path.join(root, next_dir)
            recursive_check(connection, next_path, skip_list, try_fix)


def main():
    parser = argparse.ArgumentParser("validate-query")
    parser.add_argument("-d", "--top_dir", required=True, type=str)
    parser.add_argument("-db", "--database", required=True)
    parser.add_argument("-x", "--skip", required=False, nargs="*")

    parser.add_argument("-u", "--user", required=False, default="postgres")
    parser.add_argument("-p", "--password", required=False, default="postgres")
    parser.add_argument("-H", "--host", required=False, default="127.0.0.1")
    parser.add_argument("-P", "--port", required=False, default="5432")

    parser.add_argument("--fix", action=argparse.BooleanOptionalAction, default=False)

    options: Options = parser.parse_args()
    print(options)

    connection = psycopg2.connect(
        database=options.database,
        host=options.host,
        user=options.user,
        password=options.password,
        port=options.port,
    )

    recursive_check(connection, options.top_dir, options.skip, options.fix)


if __name__ == "__main__":
    show_output = True
    main()
