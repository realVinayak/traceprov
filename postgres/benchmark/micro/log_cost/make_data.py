import argparse
from itertools import product

from traceprovpy.tools.benchmark_utils import PROVSQL_EXTRA_COMMANDS, LogCostBench
from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import ConnectionParams

def augment_provsql(table_name, cursor):
    cursor.execute("load 'provsql';")
    for extra_cmd in PROVSQL_EXTRA_COMMANDS:
        cursor.execute(extra_cmd)
    cursor.execute(f"SELECT add_provenance('{table_name}');")
    cursor.execute("SET search_path TO public;")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--provsql", action=argparse.BooleanOptionalAction, default=False)
    postgres_connection_from_cmd(parser)
    parsed = parser.parse_args()
    config = json_read_file(parsed.config)
    num_rows = config['num_rows']
    num_cols = config['num_cols']
    connection = ConnectionParams.make_simple_connection(parsed)
    cursor = connection.cursor()
    try:
        if parsed.provsql:
            cursor.execute("CREATE EXTENSION provsql CASCADE;")
            print("Created provsql extension!")
    except Exception as e:
        print("got ", e)
        cursor.execute("rollback;")
        cursor.close()
        connection.close()
        connection = ConnectionParams.make_simple_connection(parsed)
        cursor = connection.cursor()
    for (num_row, num_col) in product(num_rows, num_cols):
        create_table_stmt = LogCostBench.create_table_clause(
            num_row,
            num_col,
            include_cols=True,
            replace_table=False
        )
        columns = ",".join([f"column_{col}" for col in range(1, num_col+1)])
        table_name = LogCostBench.get_table(num_row, num_col)
        add_index_stmt = f"alter table {table_name} add primary key ({columns})"
        try:
            cursor.execute(f"drop table if exists {table_name}")
            cursor.execute(create_table_stmt)
            cursor.execute(add_index_stmt)
            cursor.execute("COMMIT;")
        except:
            cursor.execute("ROLLBACK;")
            raise
        if parsed.provsql:
            augment_provsql(table_name, cursor)
    cursor.close()
    connection.close()

if __name__ == '__main__':
    main()