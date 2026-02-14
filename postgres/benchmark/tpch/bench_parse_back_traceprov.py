from typing import List
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
    TRACEPROV_GET_DERIVATION_SPEC,
    TRACEPROV_PERFORM_DERIVATION,
    TRACEPROV_SYNC_TIME,
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


def special_query(query_name: str, is_traceprov: bool):
    if query_name != "15":
        return None
    key = "traceprov_15_skippable" if is_traceprov else "base_15_skippable"
    return Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key=key,
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
                    label="traceprov" if is_traceprov else "base",
                    query="base.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                    preprocess=([MakeTraceProv()] if is_traceprov else []),
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


def make_normal_query(query_name: str, is_traceprov=False):
    if not is_traceprov:
        return Query(
            query_name=query_name,
            spec=QuerySpec(base="base.sql", key="base"),
        )

    # don't need to check if we'll dump or not.
    extras = [TRACEPROV_SYNC_TIME(), TRACEPROV_GET_DERIVATION_SPEC()]
    return Query(
        query_name=query_name,
        spec=QuerySpec(
            base="base.sql",
            key="traceprov",
            preprocess=[MakeTraceProv()],
            extras=extras,
        ),
    )


def get_query(
    query_name: str, config: dict, use_duckdb_inference: bool, is_validate: bool
):
    user_specs = config.get("specs", [])
    subdir_queries: List[Query] = []
    if len(user_specs) == 0:
        special_query_maybe = special_query(query_name, is_traceprov=False)
        if special_query_maybe:
            subdir_queries.append(special_query_maybe)
            special_query_traceprov = special_query(query_name, is_traceprov=True)
            assert special_query_traceprov is not None
            subdir_queries.append(special_query_traceprov)
        else:
            subdir_queries.append(make_normal_query(query_name, is_traceprov=False))
            subdir_queries.append(make_normal_query(query_name, is_traceprov=True))
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

    if is_validate:
        subdir_queries.append(
            Query(
                query_name=query_name,
                spec=ValidationQuerySpec(
                    base="base.sql",
                    key="VALIDATION",
                    materialize="validate_dynamic.sql",
                ),
            )
        )
    return subdir_queries


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
            subdir_queries = [
                *subdir_queries,
                *get_query(query_name, config, use_duckdb_inference, is_validate),
            ]
        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config, layers=parsed.layers)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
