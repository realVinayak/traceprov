from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.run_with_timeout import RunParams

# query_nums = ["03", "04", "05", "06", "07"]
# query_nums = ["04", "06", "05"]
query_nums = ["03", "04", "06", "07"]

per_dir_queries = [
    [
        Query(query_name=qnum, spec=QuerySpec(base="base.sql", key="base")),
        Query(
            query_name=qnum,
            spec=QuerySpec(
                base="gprom_window.sql",
                key="gprom_window",
                materialize="gprom_window.materialize.sql",
                extras=[
                    ExtraQuery(
                        label="gprom_window_materialize_count",
                        query="$ROOT/templates/gprom/mat_count.sql",
                        runs_after_materialize=True,
                    ),
                ],
            ),
        ),
        Query(
            query_name=qnum,
            spec=QuerySpec(
                base="traceprov.sql",
                key="traceprov",
                # materialize=f"$ROOT/templates/traceprov/{qnum}.materialize.sql",
                extras=[
                    ExtraQuery(
                        label="traceprov_materialize_count",
                        query="$ROOT/templates/traceprov/mat_count.sql",
                        runs_after_materialize=True,
                    ),
                    ExtraQuery(
                        label="traceprov_infer_time",
                        query=f"$ROOT/templates/traceprov/{qnum}.infertime.sql",
                        runs_after_base=True,
                    ),
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
