import argparse

from traceprovpy.tools.benchmark import GenericBenchmark, Query, QueryDirectory, QuerySpec
from traceprovpy.tools.benchmark_utils import LogCostBench
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import RunParams

def get_base_select(num_rows, num_cols):
    columns = ','.join([f"column_{col}" for col in range(1, num_cols+1)])
    table_name = LogCostBench.get_table(num_rows, num_cols)
    select_clause = f"select {columns} from {table_name}"
    return select_clause

def get_traceprov_select(num_rows, num_cols):
    columns = ','.join([f"column_{col}::bigint" for col in range(1, num_cols+1)])
    table_name = LogCostBench.get_table(num_rows, num_cols)
    select_clause = f"select {columns}, traceprov_log_entry(1, '0'::bigint, {columns}) from {table_name}"
    return select_clause

def main():
    benchmark = GenericBenchmark("log-cost-traceprov")
    parser = argparse.ArgumentParser("driver")
    parser.add_argument("--config", required=True)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    query_dirs=[]
    num_rows = config['num_rows']
    num_cols = config['num_cols']
    for num_row in num_rows:
        queries=[]
        for num_col in num_cols:
            query_name = f"col_{num_col}"
            base_query = Query(
                query_name=query_name,
                spec=QuerySpec(base=f"$INLINE-{get_base_select(num_row, num_col)}", key="base"),
            )
            traceprov_query = Query(
                query_name=query_name,
                spec=QuerySpec(base=f"$INLINE-{get_traceprov_select(num_row, num_col)}", key="traceprov"),
            )
            queries.append(base_query)
            queries.append(traceprov_query)
        query_dirs.append(
            QueryDirectory(
                dir_name=f"row_{num_row}",
                queries=queries
            )
        )
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()
