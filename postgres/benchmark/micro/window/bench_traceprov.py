import argparse

from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import TRACEPROV_INFER_SPEC
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    MakeTraceProv,
    ReplaceSelectivity,
    RunParams,
)
from traceprovpy.tools.traceprov_extra_func import (
    TRACEPROV_LAYERS_TO_DERIVE_KEY,
    TRACEPROV_MATERIALIZE_LAYER_KEY,
)


def get_query(
    query_name: str, num_rows: list[int], materialize: bool, expanded_infer: bool
):
    def _get_query(num_row: int):
        replacer = ReplaceSelectivity(num_row, clause="NUM")
        base_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base="base.sql", key=f"base_{num_row}", preprocess=[replacer]
            ),
        )
        extras = [TRACEPROV_INFER_SPEC()]
        if expanded_infer:
            extras.append(
                ExtraQuery(
                    label="traceprov_expanded_infer",
                    query="traceprov_extended_infer.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                    strict_run=False,
                    repeat=3,
                )
            )

        traceprov_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base="base.sql",
                key=f"traceprov_{num_row}",
                preprocess=[replacer, MakeTraceProv()],
                extras=extras,
                extra_options={
                    TRACEPROV_LAYERS_TO_DERIVE_KEY: [1],
                    TRACEPROV_MATERIALIZE_LAYER_KEY: materialize,
                },
            ),
        )
        return [base_query, traceprov_query]

    return [
        single_query for num_row in num_rows for single_query in _get_query(num_row)
    ]


def main():
    benchmark = GenericBenchmark("traceprov-window")
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--mat", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument(
        "--extend_infer", action=argparse.BooleanOptionalAction, default=False
    )
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None

    dir_queries = []
    num_rows = config["num_rows"]
    for subdir in config["queries"]:
        dir_queries = [
            *dir_queries,
            *get_query(subdir, num_rows, parsed.mat, parsed.extend_infer),
        ]
    query_dir = QueryDirectory(dir_name="queries", queries=dir_queries)
    result = benchmark.run_from_argparse(
        [query_dir], RunParams(**config.get("runTimeOptions", {}))
    )
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
