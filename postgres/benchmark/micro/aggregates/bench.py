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

# query_nums = ["03", "04", "05", "06", "07"]
# query_nums = ["04", "06", "05"]
query_nums = ["03", "04", "06", "07"]


def add_when_not_eq(qnum, skip, query):
    # print(qnum, skip)
    if not qnum in skip:
        return [query]
    return []


per_dir_queries = [
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
            ["06", "07"],
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

directories = [
    QueryDirectory(
        dir_name="queries_skew_1_0_num_1000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    ),
    QueryDirectory(
        dir_name="queries_skew_1_0_num_5000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    ),
    QueryDirectory(
        dir_name="queries_skew_1_0_num_10000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    ),
    QueryDirectory(
        dir_name="queries_skew_1_0_num_50000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    ),
    QueryDirectory(
        dir_name="queries_skew_1_0_num_100000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    ),
]


def main():
    benchmark = GenericBenchmark("aggregates")
    result = benchmark.run_from_argparse(
        directories, params=RunParams(execution_time=60)
    )

    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
