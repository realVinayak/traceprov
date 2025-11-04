import argparse
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.run_with_timeout import (
    ConnectionParams,
    ReplaceFILE,
    ReplaceSelectivity,
    RunParams,
)


def get_filter_group(num_groups, selectivity, mode):
    multiplier = -1 if mode == "pre" else 1
    print(num_groups * selectivity, "num_gs")
    return int(selectivity * num_groups / 100) * multiplier


def make_directory(dir_name, mode, num_groups, selectivity, using_provsql):
    replaces_selectivity = ReplaceSelectivity(
        get_filter_group(num_groups, selectivity, mode)
    )
    extra_queries = []
    if not using_provsql:
        extra_queries = [
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
                    ],
                ),
            ),
            Query(
                query_name=f"predicate_{mode}",
                spec=QuerySpec(
                    base="traceprov_poly.sql",
                    key=f"traceprov_poly_{selectivity}",
                    preprocess=[replaces_selectivity],
                    extras=[],
                ),
            ),
        ]
    else:
        extra_queries = [
            Query(
                query_name=f"predicate_{mode}",
                spec=QuerySpec(
                    base="prov_sql_poly.sql",
                    key=f"prov_sql_selectivity_{selectivity}",
                    preprocess=[replaces_selectivity],
                ),
            ),
        ]
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
                *extra_queries,
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
        "--sel_use_provsql", action=argparse.BooleanOptionalAction, required=True
    )

    parsed, _ = parser.parse_known_args()
    print(parsed)
    assert parsed.sel_mode == "post" or parsed.sel_mode == "pre"

    # dir_names = ["1_000_000", "5_000_000", "10_000_000", "50_000_000", "100_000_000"]
    # dir_names = ["1_000_000", "5_000_000", "10_000_000", "50_000_000"]
    dir_names = ["1_000_000", "10_000_000"]
    selectivity_directories: list[QueryDirectory] = []
    for dir_name in dir_names:
        print(
            get_filter_group(
                parsed.sel_num_groups, parsed.sel_selectivity, parsed.sel_mode
            )
        )
        selectivity_directories.extend(
            make_directory(
                dir_name,
                parsed.sel_mode,
                parsed.sel_num_groups,
                parsed.sel_selectivity,
                parsed.sel_use_provsql,
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

    def _assertion(connection_params: ConnectionParams):
        assert (
            parsed.sel_use_provsql == False or "provsql" in connection_params.database
        )

    result = benchmark.run_from_argparse(
        selectivity_directories,
        params=RunParams(repeat=10, throwaway=5),
        assertions=_assertion,
        needs_setup=not parsed.sel_use_provsql
    )
    print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
