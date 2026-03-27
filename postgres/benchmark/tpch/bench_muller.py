import argparse

from traceprovpy.tools.benchmark import ExtraQuery, GenericBenchmark, Query, QueryDirectory, QuerySpec
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import RunParams

def make_query(query_name: str):
    return Query(
        query_name=query_name,
        spec=QuerySpec(
            base="phase_1.sql",
            key="phase_1_2_combined",
            materialize="phase_2.sql",
            extras=[
                ExtraQuery(
                    label="truncate_logs",
                    query="$INLINE-select truncateLogs();",
                    runs_after_materialize=True,
                    skip_validation=True,
                    strict_run=True,
                )
            ]
        )
    )

def main():
    benchmark = GenericBenchmark("tpch-driver-muller")
    parser = argparse.ArgumentParser(prog='tpch-driver')
    parser.add_argument('-cfg', '--config', required=True, type=str)
    parsed, _ = parser.parse_known_args()

    config = json_read_file(parsed.config)
    assert config is not None

    query_repr = config['queries']
    queries = []
    for query_name in map(str, query_repr):
        print(query_name)
        queries.append(make_query(query_name))

    subdir = config['subdir']

    query_dir = QueryDirectory(dir_name=subdir, queries=queries)
    result = benchmark.run_from_argparse(
        [query_dir], RunParams(**config.get("runTimeOptions", {})), init_sql=["select truncateLogs();"]
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()