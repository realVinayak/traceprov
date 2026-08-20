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
    PROVSQL_BACKUP_WRITER,
    PROVSQL_EXTRA_COMMANDS,
    PROVSQL_LOG_SIZE_SPEC,
    PROVSQL_RESTART_WRITER,
    TRACEPROV_PREPARE_INFER_SPEC,
    make_drop_table,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    TP_SKIPPABLE_OPTION,
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
    num_rows: int, is_non_factorized: bool, extra_prefix: str, file_suffix: str = "base"
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
            base=add_non_factorized_maybe(
                f"factorized_{file_suffix}.sql", is_non_factorized
            ),
            preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
        ),
    )


def get_join_dir(num_joins):
    return f"{num_joins+1}_way_join"


def get_simple_join_base_query(
    num_rows: int, num_joins: int, key: str, query_name="base.sql"
):
    return Query(
        query_name=get_join_dir(num_joins),
        spec=QuerySpec(
            key=key,
            base=query_name,
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
    def get_simple_join_query_dir(num_rows: int, num_joins: int, add_materialize: bool):
        base_query = get_simple_join_base_query(
            num_rows, num_joins, (f"base_{num_rows}"), "base.sql"
        )
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
            query_name=get_join_dir(num_joins),
            spec=QuerySpec(
                key=(f"traceprov_{num_rows}"),
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
        simple_preprocessor = [ReplaceSelectivity(num_rows, clause="NUM")]
        query_name = add_non_factorized_maybe("factor", is_non_factorized)
        normal_key = (
            add_prefix(
                add_non_factorized_maybe(
                    f"factorization_provsql_{num_rows}", is_non_factorized
                ),
                "provsql",
            ),
        )
        provsql_capture_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base=TP_SKIPPABLE_OPTION,
                key=f"{normal_key}_phase_1",
                extras=[
                    PROVSQL_BACKUP_WRITER(),
                    ExtraQuery(
                        label="capture",
                        query=add_non_factorized_maybe(
                            "factorized_base.sql", is_non_factorized
                        ),
                        runs_after_base=True,
                        skip_validation=False,
                        capture_output=False,
                        strict_run=False,
                        preprocess=[*simple_preprocessor],
                    ),
                    PROVSQL_LOG_SIZE_SPEC(),
                    PROVSQL_RESTART_WRITER(),
                ],
                preprocess=[*simple_preprocessor],
            ),
            extra_commands=[*PROVSQL_EXTRA_COMMANDS],
        )
        provsql_backtrace_all = Query(
            query_name=query_name,
            spec=QuerySpec(
                base=TP_SKIPPABLE_OPTION,
                key=f"{normal_key}_phase_1_2",
                extras=[
                    PROVSQL_BACKUP_WRITER(),
                    ExtraQuery(
                        label="capture",
                        query=add_non_factorized_maybe(
                            "factorized_provsql.sql", is_non_factorized
                        ),
                        runs_after_base=True,
                        skip_validation=False,
                        capture_output=False,
                        strict_run=False,
                        preprocess=[
                            *simple_preprocessor,
                            *([MatMaterialize(table_name)] if add_materialize else []),
                        ],
                    ),
                    PROVSQL_LOG_SIZE_SPEC(),
                    PROVSQL_RESTART_WRITER(),
                ],
                preprocess=[*simple_preprocessor],
            ),
            extra_commands=[*PROVSQL_EXTRA_COMMANDS],
        )
        # provsql_query = Query(
        #     query_name=add_non_factorized_maybe("factor", is_non_factorized),
        #     spec=QuerySpec(
        #         key=add_prefix(
        #             add_non_factorized_maybe(
        #                 f"factorization_provsql_{num_rows}", is_non_factorized
        #             ),
        #             "provsql",
        #         ),
        #         base=TP_SKIPPABLE_OPTION,
        #         base=add_non_factorized_maybe("factorized_base.sql", is_non_factorized),
        #         preprocess=[ReplaceSelectivity(num_rows, clause="NUM")],
        #         extras=[
        #             PROVSQL_BACKUP_WRITER()
        #             ExtraQuery(
        #                 label="provsql_get_polynomial",
        #                 query=add_non_factorized_maybe(
        #                     "factorized_provsql.sql", is_non_factorized
        #                 ),
        #                 runs_after_base=True,
        #                 capture_output=False,
        #                 preprocess=[
        #                     ReplaceSelectivity(num_rows, clause="NUM"),
        #                     *([MatMaterialize(table_name)] if add_materialize else []),
        #                 ],
        #             )
        #         ],
        #     ),
        #     extra_commands=["SET search_path TO provsql_test,provsql,public;"],
        # )
        queries = [provsql_capture_query, provsql_backtrace_all]
        if add_materialize:
            queries = [
                make_drop_table(
                    [table_name],
                    add_non_factorized_maybe("factor", is_non_factorized),
                    provsql_backtrace_all.extra_commands,
                ),
                *queries,
            ]
        return queries

    @staticmethod
    def get_provsql_query_all(
        query_name,
        key,
        preprocessor,
    ):
        provsql_capture_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base=TP_SKIPPABLE_OPTION,
                key=f"{key}_phase_1",
                extras=[
                    PROVSQL_BACKUP_WRITER(),
                    ExtraQuery(
                        label="capture",
                        query="base.sql",
                        runs_after_base=True,
                        skip_validation=False,
                        capture_output=False,
                        strict_run=False,
                        preprocess=[*preprocessor],
                    ),
                    PROVSQL_LOG_SIZE_SPEC(),
                    PROVSQL_RESTART_WRITER(),
                ],
                preprocess=[*preprocessor],
            ),
            extra_commands=[*PROVSQL_EXTRA_COMMANDS],
        )

        provsql_backtrace_all = Query(
            query_name=query_name,
            spec=QuerySpec(
                base=TP_SKIPPABLE_OPTION,
                key=f"{key}_phase_1_2",
                extras=[
                    PROVSQL_BACKUP_WRITER(),
                    ExtraQuery(
                        label="capture_and_backtrace",
                        query="provsql.sql",
                        runs_after_base=True,
                        skip_validation=False,
                        capture_output=False,
                        strict_run=False,
                        preprocess=[*preprocessor],
                    ),
                    PROVSQL_LOG_SIZE_SPEC(),
                    PROVSQL_RESTART_WRITER(),
                ],
                preprocess=[*preprocessor],
            ),
            extra_commands=[*PROVSQL_EXTRA_COMMANDS],
        )

        return [provsql_capture_query, provsql_backtrace_all]

    @staticmethod
    def get_simple_join_query_dir(num_rows: int, num_joins: int, add_materialize: bool):
        table_name = f"test_simple_join_{num_rows}_provsql"
        preprocessors = [ReplaceSelectivity(num_rows, clause="NUM")]

        provsql_queries = ProvSQL.get_provsql_query_all(
            get_join_dir(num_joins),
            f"provsql_{num_rows}",
            preprocessors,
        )
        queries = [*provsql_queries]
        if add_materialize:
            queries = [
                make_drop_table(
                    [table_name], "simple_join", provsql_queries[-1].extra_commands
                ),
                *queries,
            ]
        return queries


class GProM:

    @staticmethod
    def get_factorized_query_dir(
        num_rows: int, is_non_factorized: bool, add_materialize: bool
    ):
        table_name = f"test_{is_non_factorized}_{num_rows}_gprom"
        base_query = get_factorized_base_query(
            num_rows, is_non_factorized, "gprom", "gprom"
        )
        if add_materialize:
            new_spec = base_query.spec._replace(
                preprocess=[
                    *(base_query.spec.preprocess or []),
                    MatMaterialize(table_name),
                ]
            )
            base_query = base_query._replace(spec=new_spec)

        return [base_query]

    @staticmethod
    def get_simple_join_query_dir(num_rows: int, num_joins: int, add_materialize: bool):
        table_name = f"test_{num_joins}_{num_rows}_gprom"
        key = f"gprom_{num_rows}"
        base_query = get_simple_join_base_query(num_rows, num_joins, key, "gprom.sql")
        if add_materialize:
            new_spec = base_query.spec._replace(
                preprocess=[
                    *(base_query.spec.preprocess or []),
                    MatMaterialize(table_name),
                ]
            )
            base_query = base_query._replace(spec=new_spec)

        return [base_query]


def main():
    benchmark = GenericBenchmark("polynomial-driver")
    parser = argparse.ArgumentParser(prog="polynomial-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument(
        "--mode", choices=["provsql", "traceprov", "gprom"], required=True
    )
    # parser.add_argument(
    #     "--provsql", action=argparse.BooleanOptionalAction, default=False
    # )

    parser.add_argument("--mat", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--factor", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--join", action=argparse.BooleanOptionalAction, default=True)
    parsed, _ = parser.parse_known_args()
    setattr(parsed, "provsql", parsed.mode == "provsql")
    config = json_read_file(parsed.config)
    if parsed.provsql:
        benchmark = benchmark._replace(skip_load=True)

    factor_num_rows = config["factorization"]

    if parsed.mode == "provsql":
        query_class = ProvSQL
    elif parsed.mode == "traceprov":
        query_class = TraceProv
    elif parsed.mode == "gprom":
        query_class = GProM

    all_dirs = []

    if parsed.factor:
        factor_queries: list[Query] = []
        for factor_mode in [True, False]:
            for num_rows in factor_num_rows:
                factor_queries.extend(
                    query_class.get_factorized_query_dir(
                        1 << num_rows, factor_mode, add_materialize=parsed.mat
                    )
                )

        factorization_query_dir = QueryDirectory(
            dir_name="factorization", queries=factor_queries
        )

        if factorization_query_dir.queries:
            all_dirs.append(factorization_query_dir)

    if parsed.join:
        simple_join_num_rows = config["simple_join"]
        num_joins = config["num_joins"]
        simple_join_queries: list[Query] = []
        for num_rows in simple_join_num_rows:
            for num_join in num_joins:
                simple_join_queries.extend(
                    query_class.get_simple_join_query_dir(
                        num_rows, num_join, parsed.mat
                    )
                )

        simple_join_query_dir = QueryDirectory(
            dir_name="join", queries=simple_join_queries
        )
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
