from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams


test_query = [
    Query(query_name="1", spec=QuerySpec(base="base.sql", key="base")),
    Query(
        query_name="1",
        spec=QuerySpec(
            base="traceprov.sql",
            key="traceprov",
            extras=[
                ExtraQuery(
                    label="traceprov_drop_table",
                    query="$ROOT/../templates/validate_layered/1/layer_0_drop_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_drop_func",
                    query="$ROOT/../templates/validate_layered/1/layer_0_drop_function.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_create_func",
                    query="$ROOT/../templates/validate_layered/1/layer_0_create_function.sql",
                    preprocess=[ReplaceFILE("tracprov_infer_set_path")],
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                ),
                ExtraQuery(
                    label="traceprov_create_temp_table",
                    query="$ROOT/../templates/validate_layered/1/layer_0_create_temp_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                ),
                ExtraQuery(
                    label="traceprov_drop_function",
                    query="$ROOT/../templates/validate_layered/1/layer_0_drop_function.sql",
                    runs_after_base=True,
                    strict_run=True,
                    skip_validation=True,
                ),
            ],
        ),
    ),
]

directories = [QueryDirectory(dir_name="params_default", queries=test_query)]


def main():
    benchmark = GenericBenchmark("tpch-scale-1")
    result = benchmark.run_from_argparse(
        directories, params=RunParams(repeat=1, throwaway=0)
    )
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
