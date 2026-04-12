# benchmarks polynomial generation.

import argparse

from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_PREPARE_INFER_SPEC,
    make_drop_table,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    MakeTraceProv,
    MatMaterialize,
    ReplaceSelectivity,
    RunParams,
)
from traceprovpy.tools.traceprov_extra_func import (
    TRACEPROV_LAYERS_TO_DERIVE_KEY,
    TRACEPROV_MATERIALIZE_LAYER_KEY,
)


def add_non_factorized_maybe(name: str, is_non_factorized: bool):
    if is_non_factorized:
        return f"non_{name}"
    return name


def add_prefix(in_str: str, prefix: str):
    return f"{prefix}_{in_str}"


def get_factorized_base_query(
    num_rows: int, is_non_factorized: bool, extra_prefix: str
):
    return Query(
        query_name=add_non_factorized_maybe("factor", is_non_factorized),
        spec=QuerySpec(
            key=add_prefix(
                add_non_factorized_maybe(
                    f"factorization_base_{num_rows}", is_non_factorized
                ),
                extra_prefix,
            ),
            base=add_non_factorized_maybe("factorized_base.sql", is_non_factorized),
            preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
        ),
    )


def get_simple_join_base_query(num_rows: int, extra_prefix: str):
    return Query(
        query_name="simple_join",
        spec=QuerySpec(
            key=add_prefix(f"base_{num_rows}", extra_prefix),
            base="base.sql",
            preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
        ),
    )


class TraceProv:

    @staticmethod
    def get_factorized_query_dir(
        num_rows: int, is_non_factorized: bool, add_materialize: bool
    ):
        base_query = get_factorized_base_query(num_rows, is_non_factorized, "traceprov")

        extras = [TRACEPROV_PREPARE_INFER_SPEC()]
        table_name = f"test_{is_non_factorized}_{num_rows}_traceprov"
        extras.append(
            ExtraQuery(
                label="traceprov_get_polynomial",
                query=add_non_factorized_maybe(
                    "factorized_traceprov.sql", is_non_factorized
                ),
                runs_after_base=True,
                capture_output=False,
                preprocess=[
                    ReplaceSelectivity(num_rows, clause="NUM"),
                    *([MatMaterialize(table_name)] if add_materialize else []),
                ],
            )
        )
        traceprov_query = Query(
            query_name=add_non_factorized_maybe("factor", is_non_factorized),
            spec=QuerySpec(
                key=add_prefix(
                    add_non_factorized_maybe(
                        f"factorization_traceprov_{num_rows}", is_non_factorized
                    ),
                    "traceprov",
                ),
                base=add_non_factorized_maybe("factorized_base.sql", is_non_factorized),
                preprocess=[
                    ReplaceSelectivity(num_rows, clause="NUM"),
                    MakeTraceProv(),
                ],
                extras=extras,
                extra_options={
                    TRACEPROV_LAYERS_TO_DERIVE_KEY: (
                        [1] if is_non_factorized else [1, 2, 3]
                    ),
                    TRACEPROV_MATERIALIZE_LAYER_KEY: False,
                },
            ),
        )
        queries = [base_query, traceprov_query]
        if add_materialize:
            queries = [
                make_drop_table(
                    [table_name], add_non_factorized_maybe("factor", is_non_factorized)
                ),
                *queries,
            ]
        return queries

    @staticmethod
    def get_simple_join_query_dir(num_rows: int, add_materialize: bool):
        base_query = get_simple_join_base_query(num_rows, "traceprov")
        extras = [TRACEPROV_PREPARE_INFER_SPEC()]
        table_name = f"test_simple_join_{num_rows}_traceprov"
        extras.append(
            ExtraQuery(
                label="traceprov_get_polynomial",
                query="traceprov.sql",
                runs_after_base=True,
                capture_output=False,
                preprocess=[
                    ReplaceSelectivity(num_rows, clause="NUM"),
                    *([MatMaterialize(table_name)] if add_materialize else []),
                ],
            )
        )
        traceprov_query = Query(
            query_name="simple_join",
            spec=QuerySpec(
                key=add_prefix(f"traceprov_{num_rows}", "traceprov"),
                base="base.sql",
                preprocess=[
                    ReplaceSelectivity(num_rows, clause="NUM"),
                    MakeTraceProv(),
                ],
                extras=extras,
                extra_options={
                    TRACEPROV_LAYERS_TO_DERIVE_KEY: [1],
                    TRACEPROV_MATERIALIZE_LAYER_KEY: False,
                },
            ),
        )
        queries = [base_query, traceprov_query]
        if add_materialize:
            queries = [make_drop_table([table_name], "simple_join"), *queries]
        return queries


class ProvSQL:
    @staticmethod
    def get_factorized_query_dir(
        num_rows: int, is_non_factorized: bool, add_materialize: bool
    ):
        table_name = f"test_{is_non_factorized}_{num_rows}_provsql"
        provsql_query = Query(
            query_name=add_non_factorized_maybe("factor", is_non_factorized),
            spec=QuerySpec(
                key=add_prefix(
                    add_non_factorized_maybe(
                        f"factorization_provsql_{num_rows}", is_non_factorized
                    ),
                    "provsql",
                ),
                base=add_non_factorized_maybe("factorized_base.sql", is_non_factorized),
                preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
                extras=[
                    ExtraQuery(
                        label="provsql_get_polynomial",
                        query=add_non_factorized_maybe(
                            "factorized_provsql.sql", is_non_factorized
                        ),
                        runs_after_base=True,
                        capture_output=False,
                        preprocess=[
                            ReplaceSelectivity(num_rows, clause="NUM"),
                            *([MatMaterialize(table_name)] if add_materialize else []),
                        ],
                    )
                ],
            ),
            extra_commands=["SET search_path TO provsql_test,provsql,public;"],
        )
        queries = [provsql_query]
        if add_materialize:
            queries = [
                make_drop_table(
                    [table_name],
                    add_non_factorized_maybe("factor", is_non_factorized),
                    provsql_query.extra_commands,
                ),
                *queries,
            ]
        return queries

    @staticmethod
    def get_simple_join_query_dir(num_rows: int, add_materialize: bool):
        table_name = f"test_simple_join_{num_rows}_provsql"
        provsql_query = Query(
            query_name="simple_join",
            spec=QuerySpec(
                key=add_prefix(f"provsql_{num_rows}", "provsql"),
                base="base.sql",
                preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
                extras=[
                    ExtraQuery(
                        label="provsql_get_polynomial",
                        query="provsql.sql",
                        runs_after_base=True,
                        capture_output=False,
                        preprocess=[
                            ReplaceSelectivity(num_rows, clause="NUM"),
                            *([MatMaterialize(table_name)] if add_materialize else []),
                        ],
                    )
                ],
            ),
            extra_commands=["SET search_path TO provsql_test,provsql,public;"],
        )
        queries = [provsql_query]
        if add_materialize:
            queries = [
                make_drop_table(
                    [table_name], "simple_join", provsql_query.extra_commands
                ),
                *queries,
            ]
        return queries


def main():
    benchmark = GenericBenchmark("polynomial-driver")
    parser = argparse.ArgumentParser(prog="polynomial-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument(
        "--provsql", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--mat", action=argparse.BooleanOptionalAction, default=False)

    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    if parsed.provsql:
        benchmark = benchmark._replace(skip_load=True)

    factor_num_rows = config["factorization"]
    factor_queries: list[Query] = []
    query_class = ProvSQL if parsed.provsql else TraceProv
    for num_rows in factor_num_rows:
        factor_queries.extend(
            query_class.get_factorized_query_dir(
                1 << num_rows, True, add_materialize=parsed.mat
            )
        )
        factor_queries.extend(
            query_class.get_factorized_query_dir(
                1 << num_rows, False, add_materialize=parsed.mat
            )
        )

    factorization_query_dir = QueryDirectory(
        dir_name="factorization", queries=factor_queries
    )

    all_dirs = []
    if factorization_query_dir.queries:
        all_dirs.append(factorization_query_dir)

    simple_join_num_rows = config["simple_join"]
    simple_join_queries: list[Query] = []
    for num_rows in simple_join_num_rows:
        simple_join_queries.extend(
            query_class.get_simple_join_query_dir(num_rows, parsed.mat)
        )

    simple_join_query_dir = QueryDirectory(dir_name="join", queries=simple_join_queries)
    if simple_join_query_dir.queries:
        all_dirs.append(simple_join_query_dir)

    assert len(all_dirs) > 0, "Expected some config params!"
    result = benchmark.run_from_argparse(
        all_dirs, RunParams(**config.get("runTimeOptions", {}))
    )
    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
