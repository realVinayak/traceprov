from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.run_with_timeout import ReplaceSelectivity, RunParams

directories = [
    QueryDirectory(
        dir_name="1_000_000",
        queries=[
            Query(
                query_name="predicate_post",
                spec=QuerySpec(
                    base="base.sql",
                    key="base_selectivity_10",
                    preprocess=[ReplaceSelectivity((1))],
                ),
            ),
            # Query(
            #     query_name="predicate_post",
            #     spec=QuerySpec(
            #         base="gprom_window.sql", key="gprom_window_selecivity_10"
            #     ),
            # ),
        ],
    )
]


def main():
    benchmark = GenericBenchmark("selectivity")
    result = benchmark.run_from_argparse(
        directories, params=RunParams(repeat=2, throwaway=2)
    )
    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
