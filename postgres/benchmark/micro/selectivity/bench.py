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


def get_filter_group(num_groups, selectivity):
    return int(selectivity * num_groups / 100)


directories = lambda num_groups, selectivity: [
    QueryDirectory(
        dir_name="1_000_000",
        queries=[
            Query(
                query_name="predicate_post",
                spec=QuerySpec(
                    base="base.sql",
                    key=f"base_selectivity_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
                    ],
                ),
            ),
            Query(
                query_name="predicate_post",
                spec=QuerySpec(
                    base="gprom_join.sql",
                    materialize="gprom_join.materialize.sql",
                    key=f"gprom_join_selectivity_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
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
                query_name="predicate_post",
                spec=QuerySpec(
                    base="gprom_window.sql",
                    materialize="gprom_window.materialize.sql",
                    key=f"gprom_window_selectivity_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
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
                query_name="predicate_post",
                spec=QuerySpec(
                    base="gprom_join_heuristics.sql",
                    materialize="gprom_join_heuristics.materialize.sql",
                    key=f"gprom_join_heuristics_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
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
                query_name="predicate_post",
                spec=QuerySpec(
                    base="gprom_window_heuristics.sql",
                    materialize="gprom_window_heuristics.materialize.sql",
                    key=f"gprom_window_heuristics_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
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
                query_name="predicate_post",
                spec=QuerySpec(
                    base="traceprov.sql",
                    key=f"traceprov_{selectivity}",
                    preprocess=[
                        ReplaceSelectivity(get_filter_group(num_groups, selectivity))
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
                            query=f"$ROOT/templates/predicate_post/traceprov_create_function.sql",
                            runs_after_base=True,
                            strict_run=True,
                            skip_validation=True,
                            preprocess=[ReplaceFILE("traceprov_infer_set_path")],
                        ),
                        ExtraQuery(
                            label="traceprov_materialize",
                            query=f"$ROOT/templates/predicate_post/traceprov_materialize.sql",
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
    parsed, _ = parser.parse_known_args()
    print(parsed)
    selectivity_directories = directories(parsed.sel_num_groups, parsed.sel_selectivity)
    result = benchmark.run_from_argparse(
        selectivity_directories, params=RunParams(repeat=2, throwaway=2)
    )
    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
