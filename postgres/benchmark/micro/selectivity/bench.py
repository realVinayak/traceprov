import argparse
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.run_with_timeout import (
    ReplaceFILE,
    ReplaceSelectivity,
    RunParams,
)


def get_filter_group(num_groups, selectivity, mode):
    multiplier = -1 if mode == "pre" else 1
    return int(selectivity * num_groups / 100) * multiplier


def make_directory(dir_name, mode, num_groups, selectivity):
    return [
        QueryDirectory(
            dir_name=dir_name,
            queries=[
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="base.sql",
                        key=f"base_selectivity_{selectivity}",
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
                    ),
                ),
                Query(
                    query_name=f"predicate_{mode}",
                    spec=QuerySpec(
                        base="gprom_join.sql",
                        materialize="gprom_join.materialize.sql",
                        key=f"gprom_join_selectivity_{selectivity}",
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
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
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
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
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
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
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
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
                        preprocess=[
                            ReplaceSelectivity(
                                get_filter_group(num_groups, selectivity, mode)
                            )
                        ],
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
    parser.add_argument("--sel_selectivity", required=True, type=int)
    parser.add_argument("--sel_mode", required=True, type=str)

    parsed, _ = parser.parse_known_args()
    print(parsed)
    assert parsed.sel_mode == "post" or parsed.sel_mode == "pre"
    #dir_names = ["1_000_000", "5_000_000", "10_000_000", "50_000_000", "100_000_000"]
    dir_names = ["1_000_000", "5_000_000", "10_000_000", "50_000_000"]
    selectivity_directories = []
    for dir_name in dir_names:
        selectivity_directories.extend(
            make_directory(
                dir_name,
                parsed.sel_mode,
                parsed.sel_num_groups,
                parsed.sel_selectivity,
            )
        )

    result = benchmark.run_from_argparse(
        selectivity_directories, params=RunParams(repeat=2, throwaway=2)
    )
    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
