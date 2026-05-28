from pathlib import Path

import sqlglot
from sqlglot.optimizer.scope import build_scope
import argparse

from traceprovpy.tools.file_utils import just_read, just_write

select_table_mapping = None


def transform(node):
    assert select_table_mapping is not None
    if (
        isinstance(node, sqlglot.exp.Alias)
        and isinstance(node.this, sqlglot.exp.Column)
        and node.alias.lower().startswith("prov")
        and not node.this.this.this.lower().startswith("prov")
    ):
        parent_select = node.parent_select
        tables = select_table_mapping[parent_select]
        print(node.this.table, tables)
        if node.this.table in tables:
            print("Need replacing: ", node)
            return sqlglot.alias(sqlglot.column("rowid", node.this.table), node.alias)
    # print(type(node))
    return node


def transform_key_to_rowid(ast):
    root = build_scope(ast)
    hashed = dict()
    for scope in root.traverse():
        # print(repr(scope.expression.expressions))
        # print(scope.selected_sources)
        tables = scope.selected_sources
        hashed[scope.expression] = [
            alias
            for alias, (node, source) in scope.selected_sources.items()
            if isinstance(source, sqlglot.exp.Table)
        ]
        # for entry in scope.expression.expressions:
        #     if not isinstance(entry, sqlglot.exp.Alias):
        #         continue
        #     print(type(entry.alias))
        #     # print(repr(entry))

        # # print(repr(scope.expression.expressions))
        # # print(repr(scope.selected_sources))
    return hashed


def _transform(file: Path):
    content = just_read(file)
    ast = sqlglot.parse_one(content, dialect="duckdb")
    res = transform_key_to_rowid(ast)
    global select_table_mapping
    select_table_mapping = res
    print(res.values())
    transformed = ast.transform(transform)
    sql = transformed.sql(dialect="duckdb")
    just_write(file, sql)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file", required=True)
    parsed = parser.parse_args()
    _transform(parsed.file)


if __name__ == "__main__":
    main()
