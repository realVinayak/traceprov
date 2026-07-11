import argparse
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    bench_has_duckdb_infer,
)
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_GET_DERIVATION_SPEC,
    TRACEPROV_GET_LAYER_SIZE,
    TRACEPROV_INFER_SPEC,
    TRACEPROV_SYNC_TIME,
)
from traceprovpy.tools.duckdb_inference import DuckDBInferenceQuerySpec
from traceprovpy.tools.run_with_timeout import (
    MakeTraceProv,
    ReplaceFILE,
    ReplaceSelectivity,
    RunParams,
)
from traceprovpy.tools.traceprov_extra_func import TRACEPROV_LAYERS_TO_DERIVE_KEY
from traceprovpy.utils import get_filter_group


def make_simple_directory(
    dir_name, mode, num_groups, selectivity, use_duckdb_inference: bool, use_random
):
    replaces_selectivity = ReplaceSelectivity(
        get_filter_group(num_groups, selectivity, mode)
    )
    new_name = f"data_table_{dir_name}"
    if use_random:
        new_name = f"data_table_{dir_name}_random"
    replaces_random = ReplaceSelectivity(
        new_name, f"data_table_{dir_name}"
    )
    # print(replaces_random)
    # assert False

    make_base = lambda query_name: Query(
        query_name=f"predicate_{mode}",
        spec=QuerySpec(
            base=f"{query_name}.sql",
            key=f"{query_name}_selectivity_{selectivity}",
            preprocess=[replaces_selectivity, replaces_random],
        ),
    )
    traceprov_extras = [TRACEPROV_INFER_SPEC(), TRACEPROV_GET_LAYER_SIZE()]
    traceprov_query = Query(
        query_name=f"predicate_{mode}",
        spec=QuerySpec(
            base="base.sql",
            key=f"traceprov_selectivity_{selectivity}",
            preprocess=[replaces_selectivity, replaces_random, MakeTraceProv()],
            extras=traceprov_extras,
            extra_options={
                TRACEPROV_LAYERS_TO_DERIVE_KEY: (1,)
            },
        ),
    )

    dir_queries = [
        make_base("base"),
        # make_base("gprom_join"),
        # make_base("gprom_join_heuristics"),
        # make_base("gprom_window"),
        make_base("gprom_window_heuristics"),
        traceprov_query,
    ]
    if use_duckdb_inference:
        dir_queries.append(
            Query(
                query_name=f"predicate_{mode}",
                spec=DuckDBInferenceQuerySpec(
                    base="DUCKDB_INFERENCE",
                    key=f"DUCKDB_INFERENCE_predicate_{mode}_{selectivity}",
                ),
            )
        )
    return [
        QueryDirectory(
            dir_name=dir_name,
            queries=dir_queries,
        )
    ]


def make_directory(dir_name, mode, num_groups, selectivity):
    replaces_selectivity = ReplaceSelectivity(
        get_filter_group(num_groups, selectivity, mode)
    )
    return [
        QueryDirectory(
            dir_name=dir_name,
            queries=[
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="base.sql",
                        key=f"base_selectivity_{selectivity}",
                        preprocess=[replaces_selectivity],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="gprom_join.sql",
                        materialize="gprom_join.materialize.sql",
                        key=f"gprom_join_selectivity_{selectivity}",
                        preprocess=[replaces_selectivity],
                        extras=[
                            ExtraQuery(
                                label="count",
                                query=f"$INLINE-select count(*) from gprom_lineage;",
                                runs_after_materialize=True,
                                strict_run=True,
                                skip_validation=True,
                            )
                        ],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="gprom_window.sql",
                        materialize="gprom_window.materialize.sql",
                        key=f"gprom_window_selectivity_{selectivity}",
                        preprocess=[replaces_selectivity],
                        extras=[
                            ExtraQuery(
                                label="count",
                                query=f"$INLINE-select count(*) from gprom_lineage;",
                                runs_after_materialize=True,
                                strict_run=True,
                                skip_validation=True,
                            )
                        ],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="gprom_join_heuristics.sql",
                        materialize="gprom_join_heuristics.materialize.sql",
                        key=f"gprom_join_heuristics_{selectivity}",
                        preprocess=[replaces_selectivity],
                        extras=[
                            ExtraQuery(
                                label="count",
                                query=f"$INLINE-select count(*) from gprom_lineage;",
                                runs_after_materialize=True,
                                strict_run=True,
                                skip_validation=True,
                            )
                        ],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="gprom_window_heuristics.sql",
                        materialize="gprom_window_heuristics.materialize.sql",
                        key=f"gprom_window_heuristics_{selectivity}",
                        preprocess=[replaces_selectivity],
                        extras=[
                            ExtraQuery(
                                label="count",
                                query=f"$INLINE-select count(*) from gprom_lineage;",
                                runs_after_materialize=True,
                                strict_run=True,
                                skip_validation=True,
                            )
                        ],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="traceprov.sql",
                        key=f"traceprov_{selectivity}",
                        preprocess=[replaces_selectivity],
                        extras=[
                            ExtraQuery(
                                label="traceprov_sync_time",
                                query=f"$INLINE-select * from traceprov_sync_time(0);",
                                runs_after_base=True,
                                strict_run=True,
                            ),
                            # ExtraQuery(
                            #     label="traceprov_get_tables_before",
                            #     query=f"$INLINE-SELECT table_name FROM information_schema.tables;",
                            #     runs_after_base=True,
                            #     strict_run=True,
                            #     skip_validation=True,
                            # ),
                            ExtraQuery(
                                label="traceprov_drop_table",
                                query=f"$INLINE-DROP TABLE IF EXISTS traceprov_lineage_selectivity;",
                                runs_after_base=True,
                                strict_run=True,
                                skip_validation=True,
                            ),
                            # ExtraQuery(
                            #     label="traceprov_get_tables_later",
                            #     query=f"$INLINE-SELECT table_name FROM information_schema.tables;",
                            #     runs_after_base=True,
                            #     strict_run=True,
                            #     skip_validation=True,
                            # ),
                            ExtraQuery(
                                label="traceprov_drop_function",
                                query=f"$INLINE-DROP FUNCTION IF EXISTS traceprov_infer_selectivity_bench;",
                                runs_after_base=True,
                                strict_run=True,
                                skip_validation=True,
                            ),
                            ExtraQuery(
                                label="traceprov_create_function",
                                query=f"$ROOT/templates/traceprov_create_function.sql",
                                runs_after_base=True,
                                strict_run=True,
                                skip_validation=True,
                                preprocess=[ReplaceFILE("traceprov_infer_set_path")],
                            ),
                            ExtraQuery(
                                label="traceprov_materialize",
                                query=f"$ROOT/templates/traceprov_materialize.sql",
                                runs_after_base=True,
                                skip_validation=True,
                                capture_output=False,
                            ),
                            ExtraQuery(
                                label="traceprov_infer_time",
                                query=f"$INLINE-select * from traceprov_infer_time(1, 0, 0);",
                                runs_after_base=True,
                                strict_run=True,
                                skip_validation=True,
                            ),
                            ExtraQuery(
                                label="traceprov_infer_count",
                                query=f"$INLINE-select count(*) from traceprov_lineage_selectivity;",
                                runs_after_base=True,
                                strict_run=True,
                                skip_validation=True,
                            ),
                        ],
                    ),
                ),
            ],
        )
    ]


def main():
    benchmark = GenericBenchmark("selectivity")
    parser = argparse.ArgumentParser("selectivity-driver")
    parser.add_argument("--sel_num_groups", required=True, type=int)
    parser.add_argument("--sel_selectivity", required=True, type=float)
    parser.add_argument("--sel_mode", required=True, type=str)
    parser.add_argument(
        "--sel_dry_run", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--random", action=argparse.BooleanOptionalAction, default=True
    )

    parsed, others = parser.parse_known_args()
    print(parsed)
    assert parsed.sel_mode == "post" or parsed.sel_mode == "pre"

    # dir_names = ["1_000_000", "5_000_000", "10_000_000", "50_000_000", "100_000_000"]
    dir_names = ["1_000_000"]
    selectivity_directories: list[QueryDirectory] = []
    for dir_name in dir_names:
        print(
            get_filter_group(
                parsed.sel_num_groups, parsed.sel_selectivity, parsed.sel_mode
            )
        )
        selectivity_directories.extend(
            make_simple_directory(
                dir_name,
                parsed.sel_mode,
                parsed.sel_num_groups,
                parsed.sel_selectivity,
                use_duckdb_inference=bench_has_duckdb_infer(others),
                use_random=parsed.random
            )
        )

    # As part of validation, also need to check that all the keys are distinct.
    # In general, that may not be true.
    keys = [
        [q.spec.key for q in directory.queries] for directory in selectivity_directories
    ]

    for key in keys:
        assert len(key) == len(set(key)), f"mismatch: {key}"

    if parsed.sel_dry_run:
        print(selectivity_directories)
        return
    result = benchmark.run_from_argparse(
        selectivity_directories, params=RunParams()
    )
    # print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
