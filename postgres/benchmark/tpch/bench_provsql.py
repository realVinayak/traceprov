import argparse

from traceprovpy.tools.benchmark import ExtraQuery, GenericBenchmark, Query, QueryDirectory, QuerySpec
from traceprovpy.tools.benchmark_utils import PROVSQL_BACKUP_WRITER, PROVSQL_EXTRA_COMMANDS, PROVSQL_LOG_SIZE_SPEC, PROVSQL_RESTART_WRITER, parse_queries
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, RunParams


def get_query(query_name: str, parsed):
    query_phase_1 = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="phase_1",
            extras=[
                PROVSQL_BACKUP_WRITER(),
                ExtraQuery(
                    label="capture",
                    query="base.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                ),
                PROVSQL_LOG_SIZE_SPEC(),
                PROVSQL_RESTART_WRITER()
            ]
        ),
        extra_commands=[*PROVSQL_EXTRA_COMMANDS]
    )
    query_phase_1_2 = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="phase_1_2",
            extras=[
                PROVSQL_BACKUP_WRITER(),
                ExtraQuery(
                    label="capture_and_backtrace",
                    query="provsql_capture_backtrace.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                ),
                PROVSQL_LOG_SIZE_SPEC(),
                PROVSQL_RESTART_WRITER()
            ]
        ),
        extra_commands=[*PROVSQL_EXTRA_COMMANDS]
    )
    return [query_phase_1, query_phase_1_2]

def main():
    benchmark = GenericBenchmark("provsql-tpch-driver")
    parser = argparse.ArgumentParser(prog='provsql')
    parser.add_argument("--config", required=True, type=str)
    parser.add_argument(
        "--bulk_derive", action=argparse.BooleanOptionalAction, default=True
    )
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    queries = config["queries"]
    queries = parse_queries(queries)
    dir_queries = []
    for subdir in config['subdirs']:
        subdir_queries = []
        for query_name in queries:
            query_name = str(query_name)
            subdir_queries = [
                *subdir_queries,
                *get_query(query_name, parsed)
            ]
        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    benchmark = benchmark._replace(skip_load=True)
    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()