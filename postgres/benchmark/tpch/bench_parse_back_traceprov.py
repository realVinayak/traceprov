from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    ValidationQuerySpec,
    bench_has_duckdb_infer,
)
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_CAPTURE_QUERY,
    TRACEPROV_PERFORM_DERIVATION,
)
from traceprovpy.tools.duckdb_inference import DuckDBInferenceQuerySpec
from traceprovpy.tools.run_with_timeout import (
    TP_SKIPPABLE_OPTION,
    MakeTraceProv,
    ReplaceFILE,
    RunParams,
)
import json
import argparse


def special_query(query_name: str):
    if query_name != "15":
        return None
    return Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="traceprov_15_skippable",
            extras=[
                ExtraQuery(
                    label="15_post",
                    query="$INLINE-drop view if exists revenue0;",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="15_pre",
                    query="create_view.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov",
                    query="base.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                    preprocess=[MakeTraceProv()],
                ),
                TRACEPROV_CAPTURE_QUERY(),
                ExtraQuery(
                    label="15_post",
                    query="$INLINE-drop view revenue0;",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                    strict_run=True,
                ),
            ],
        ),
    )


# This parses out the config file, and generates the directories
# on the fly, to be used.
# We log the directories later, anyways, so this is fine.
def main():
    benchmark = GenericBenchmark("tpch-driver")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-l", "--layers", required=True, type=str)
    parsed, others = parser.parse_known_args()
    with open(parsed.config) as f:
        config: dict = json.loads(f.read())
    is_validate = config.get("validate", False)
    use_duckdb_inference = bench_has_duckdb_infer(others)
    dir_queries = []

    for subdir in config["subdirs"]:
        subdir_queries = []
        for query_name in config["queries"]:
            query_name = str(query_name)
            user_specs = config.get("specs", [])
            if len(user_specs) == 0:
                special_query_maybe = special_query(query_name)
                if special_query_maybe:
                    subdir_queries.append(special_query_maybe)
                else:
                    subdir_queries.append(
                        Query(
                            query_name=query_name,
                            spec=QuerySpec(
                                base="base.sql",
                                key="traceprov",
                                preprocess=[MakeTraceProv()],
                                extras=[
                                    # TRACEPROV_CAPTURE_QUERY()
                                ],
                            ),
                        ),
                    )
                    if use_duckdb_inference:
                        subdir_queries.append(
                            Query(
                                query_name=query_name,
                                spec=DuckDBInferenceQuerySpec(
                                    base="DUCKDB_INFERENCE",
                                    key=f"DUCKDB_INFERENCE_{query_name}",
                                ),
                            )
                        )
                    # subdir_queries.append(
                    #     Query(
                    #         query_name=query_name,
                    #         spec=CopyColumnCsv(
                    #             "copy_traceprov_csv",
                    #             base="base.sql",
                    #             materialize=f"traceprov_infer_dump/{subdir}/{query_name}/",
                    #         ),
                    #     )
                    # )
            else:
                for spec in user_specs:
                    spec_without_extras = {
                        key: value for (key, value) in spec.items() if key != "extras"
                    }
                    extras = [ExtraQuery(**kwargs) for kwargs in spec.get("extras", [])]
                    subdir_queries.append(
                        Query(
                            query_name=query_name,
                            spec=QuerySpec(**spec_without_extras, extras=extras),
                        ),
                    )

        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config, layers=parsed.layers)
    benchmark.dump_final_result(result)


import os


class CopyColumnCsv(QuerySpec):
    def run_packs(self, top_dir, get_run_options):
        assert os.system(f"mkdir -p {self.materialize}") == 0
        assert (
            os.system(
                f"sudo cp /var/lib/postgresql/14/main/traceprov/2_dump.csv {self.materialize}/"
            )
            == 0
        )


if __name__ == "__main__":
    main()
