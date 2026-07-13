import argparse
from itertools import product

from traceprovpy.tools.benchmark_utils import LogCostBench
from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import ConnectionParams


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    postgres_connection_from_cmd(parser)
    parsed = parser.parse_args()

    config = json_read_file(parsed.config)
    num_rows = config['num_rows']
    num_cols = config['num_cols']
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
    cursor.close()
    connection.close()

if __name__ == '__main__':
    main()