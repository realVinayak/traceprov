import argparse
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    GPROM_LINEAGE_COUNT,
    TRACEPROV_DROP_FUNCTION,
    TRACEPROV_DROP_TABLE,
    TRACEPROV_INFER_COUNT,
    TRACEPROV_INFER_TIME,
    TRACEPROV_SYNC_TIME,
)
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams


TOP_LEVEL_QUERIES = ["03", "04", "06", "07"]
# TOP_LEVEL_QUERIES = ["03"]
# These queries should be looked into db_* (specified by -gnum)
PER_DB_QUERIES = ["08"]

ALL_QUERIES = [TOP_LEVEL_QUERIES, PER_DB_QUERIES]


def add_when_not_eq(qnum, skip, query):
    # print(qnum, skip)
    if not qnum in skip:
        return [query]
    return []


per_dir_queries = [
    [
        [
            Query(query_name=qnum, spec=QuerySpec(base="base.sql", key="base")),
            Query(
                query_name=qnum,
                spec=QuerySpec(
                    base="gprom_join.sql",
                    key="gprom_join",
                    materialize="gprom_join.materialize.sql",
                    extras=[GPROM_LINEAGE_COUNT()],
                ),
            ),
            Query(
                query_name=qnum,
                spec=QuerySpec(
                    base="gprom_window.sql",
                    key="gprom_window",
                    materialize="gprom_window.materialize.sql",
                    extras=[GPROM_LINEAGE_COUNT()],
                ),
            ),
            *add_when_not_eq(
                qnum,
                ["06", "07", "08"],
                Query(
                    query_name=qnum,
                    spec=QuerySpec(
                        base="gprom_join_heuristics.sql",
                        key="gprom_join_heuristics",
                        materialize="gprom_join_heuristics.materialize.sql",
                        extras=[GPROM_LINEAGE_COUNT()],
                    ),
                ),
            ),
            Query(
                query_name=qnum,
                spec=QuerySpec(
                    base="gprom_window_heuristics.sql",
                    key="gprom_window_heuristics",
                    materialize="gprom_window_heuristics.materialize.sql",
                    extras=[GPROM_LINEAGE_COUNT()],
                ),
            ),
            Query(
                query_name=qnum,
                spec=QuerySpec(
                    base="traceprov.sql",
                    key="traceprov",
                    extras=[
                        TRACEPROV_SYNC_TIME(),
                        TRACEPROV_DROP_TABLE(),
                        TRACEPROV_DROP_FUNCTION(),
                        ExtraQuery(
                            label="traceprov_create_function",
                            query=f"$ROOT/templates/traceprov/traceprov_create_function.sql",
                            runs_after_base=True,
                            strict_run=True,
                            skip_validation=True,
                            preprocess=[ReplaceFILE("traceprov_infer_set_path")],
                        ),
                        ExtraQuery(
                            label="traceprov_materialize",
                            query=f"$ROOT/templates/traceprov/traceprov_materialize.sql",
                            runs_after_base=True,
                            skip_validation=True,
                            capture_output=False,
                        ),
                        TRACEPROV_INFER_TIME(),
                        TRACEPROV_INFER_COUNT(),
                    ],
                ),
            ),
        ]
        for qnum in query_nums
    ]
    for query_nums in ALL_QUERIES
]

get_directories = lambda gnum, index: [
    QueryDirectory(
        dir_name=f"./{gnum}/queries_skew_1_0_num_1000000",
        queries=[q for per_dir in per_dir_queries[index] for q in per_dir],
    ),
    QueryDirectory(
        dir_name=f"./{gnum}/queries_skew_1_0_num_5000000",
        queries=[q for per_dir in per_dir_queries[index] for q in per_dir],
    ),
    QueryDirectory(
        dir_name=f"./{gnum}/queries_skew_1_0_num_10000000",
        queries=[q for per_dir in per_dir_queries[index] for q in per_dir],
    ),
    QueryDirectory(
        dir_name=f"./{gnum}/queries_skew_1_0_num_50000000",
        queries=[q for per_dir in per_dir_queries[index] for q in per_dir],
    ),
    QueryDirectory(
        dir_name=f"./{gnum}/queries_skew_1_0_num_100000000",
        queries=[q for per_dir in per_dir_queries[index] for q in per_dir],
    ),
]


def main():
    benchmark = GenericBenchmark("aggregates")
    parser = argparse.ArgumentParser("aggregates-driver")
    parser.add_argument("-agg_gnum", "--agg_gnum", required=False, default="", type=str)
    parsed, _ = parser.parse_known_args()
    top_dirs = get_directories("", 0)
    per_db_dirs = get_directories(parsed.agg_gnum, 1)
    directories = [*top_dirs, *per_db_dirs]
    # filter out unused dirs (happens when we add per-db later for testing)
    directories = [_dir for _dir in directories if len(_dir.queries) > 0]
    result = benchmark.run_from_argparse(
        directories, params=RunParams(execution_time=60, repeat=3, throwaway=0)
    )

    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
