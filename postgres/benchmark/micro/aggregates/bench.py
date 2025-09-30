from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)


benchmark = GenericBenchmark("aggregates")


query_nums = ["03", "04", "05", "06", "07"]

per_dir_queries = [
    [
        Query(query_name=qnum, spec=QuerySpec(base="base.sql", key="base")),
        Query(
            query_name=qnum,
            spec=QuerySpec(
                base="gprom_window.sql",
                key="gprom_window",
                materialize="gprom_window.materialize.sql",
            ),
        ),
    ]
    for qnum in query_nums
]
directories = [
    QueryDirectory(
        dir_name="queries_skew_1_0_num_1000000",
        queries=[q for per_dir in per_dir_queries for q in per_dir],
    )
]

result = benchmark.run(
    "postgres", "postgres", "microbench_agg_02_10", "./", directories
)

print(result)
