from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    ValidationQuerySpec,
)
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams


simple_per_query = lambda query_name: [
    Query(query_name=query_name, spec=QuerySpec(base="base.sql", key="base")),
    Query(
        query_name=query_name,
        spec=QuerySpec(
            base="traceprov.sql",
            key="traceprov",
            extras=[
                ExtraQuery(
                    label="traceprov_sync_time",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_sync_time.sql",
                    runs_after_base=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_drop_table",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_drop_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_drop_func",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_drop_function.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_create_func",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_create_function.sql",
                    preprocess=[ReplaceFILE("traceprov_infer_set_path")],
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_create_temp_table",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_create_temp_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                ),
                ExtraQuery(
                    label="traceprov_drop_function",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_drop_function.sql",
                    runs_after_base=True,
                    strict_run=True,
                    skip_validation=True,
                ),
                ExtraQuery(
                    label="traceprov_infer_time",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_infer_time.sql",
                    runs_after_base=True,
                    strict_run=True,
                ),
            ],
        ),
    ),
    Query(
        query_name=query_name,
        spec=ValidationQuerySpec(
            base="base.sql", key="VALIDATION", materialize="lineage_restricted.sql"
        ),
    ),
]

directories = [QueryDirectory(dir_name="params_default", queries=simple_per_query("3"))]


def main():
    benchmark = GenericBenchmark("tpch-scale-1")
    result = benchmark.run_from_argparse(
        directories, params=RunParams(repeat=1, throwaway=0)
    )
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
