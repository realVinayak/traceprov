import argparse

from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    PROVSQL_BACKUP_WRITER,
    PROVSQL_EXTRA_COMMANDS,
    PROVSQL_LOG_SIZE_SPEC,
    PROVSQL_RESTART_WRITER,
    LogCostBench,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, RunParams


def get_provsql_select(num_rows, num_cols):
    table_name = LogCostBench.get_table(num_rows, num_cols)
    select_clause = f"select provsql.create_gate(uuid_generate_v5(provsql.uuid_ns_provsql(), 'test'), 'project', ARRAY[provsql]) from {table_name}"
    return select_clause


def main():
    benchmark = GenericBenchmark("log-cost-traceprov")
    parser = argparse.ArgumentParser("driver")
    parser.add_argument("--config", required=True)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    query_dirs = []
    num_rows = config["num_rows"]
    num_cols = config["num_cols"]
    for num_row in num_rows:
        queries = []
        for num_col in num_cols:
            query_name = f"col_{num_col}"
            provsql_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base=TP_SKIPPABLE_OPTION,
                    key="provsql",
                    extras=[
                        PROVSQL_BACKUP_WRITER(),
                        ExtraQuery(
                            label="provsql",
                            query=f"$INLINE-{get_provsql_select(num_row, num_col)}",
                            runs_after_base=True,
                            skip_validation=True,
                            capture_output=False,
                            strict_run=False,
                        ),
                        PROVSQL_LOG_SIZE_SPEC(),
                        PROVSQL_RESTART_WRITER(),
                    ],
                ),
                extra_commands=[
                    "SET provsql.active = false;",
                    *PROVSQL_EXTRA_COMMANDS,
                ],
            )
            queries.append(provsql_query)
        query_dirs.append(
            QueryDirectory(dir_name=f"provsql_row_{num_row}", queries=queries)
        )
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
