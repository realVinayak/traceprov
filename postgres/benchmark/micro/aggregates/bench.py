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
query_nums = ["04", "06", "07"]

RUN_IS_MATERIALIZE = lambda _, b_or_m: b_or_m == "materialize"
RUN_IS_BASE = lambda _, b_or_m: b_or_m == "base"

per_dir_queries = [
    [
        Query(query_name=qnum, spec=QuerySpec(base="base.sql", key="base")),
        # Query(
        #    query_name=qnum,
        #    spec=QuerySpec(
        #        base="gprom_window.sql",
        #        key="gprom_window",
        #        materialize="gprom_window.materialize.sql",
        #        extras=[
        #            ExtraQuery(
        #                label="gprom_window_materialize_count",
        #                query="$ROOT/templates/gprom/mat_count.sql",
        #                should_run=RUN_IS_MATERIALIZE,
        #            ),
        #        ],
        #    ),
        # ),
        Query(
            query_name=qnum,
            spec=QuerySpec(
                base="traceprov.sql",
                key="traceprov",
                # materialize=f"$ROOT/templates/traceprov/{qnum}.materialize.sql",
                extras=[
                    # ExtraQuery(
                    #    label="traceprov_materialize_count",
                    #    query="$ROOT/templates/traceprov/mat_count.sql",
                    #    should_run=RUN_IS_MATERIALIZE,
                    # ),
                    ExtraQuery(
                        label="traceprov_infer_time",
                        query=f"$ROOT/templates/traceprov/{qnum}.infertime.sql",
                        should_run=RUN_IS_BASE,
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

import json
from datetime import datetime


def main():
    current_datetime = datetime.now()
    timestamp_string = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")
    benchmark = GenericBenchmark("aggregates")
    result = benchmark.run_from_argparse(
        directories, params=RunParams(execution_time=60)
    )

    print(result)
    with open(f"result_{timestamp_string}.json", "w") as f:
        f.write(json.dumps(result, indent=4))


if __name__ == "__main__":
    main()
