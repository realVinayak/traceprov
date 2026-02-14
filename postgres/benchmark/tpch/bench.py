from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    ValidationQuerySpec,
    bench_has_duckdb_infer,
    bench_has_smokedduck,
)
from traceprovpy.tools.duckdb_inference import DuckDBInferenceQuerySpec
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams
import json
import argparse

from traceprovpy.tools.smokedduck import SmokedDuckQuerySpec


# This parses out the config file, and generates the directories
# on the fly, to be used.
# We log the directories later, anyways, so this is fine.
def main():
    benchmark = GenericBenchmark("tpch-driver")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-l", "--layers", required=True, type=str)
    parser.add_argument(
        "--traceprov", action=argparse.BooleanOptionalAction, default=True
    )
    parsed, others = parser.parse_known_args()
    with open(parsed.config) as f:
        config: dict = json.loads(f.read())
    is_validate = config.get("validate", False)

    has_sd = bench_has_smokedduck(others)
    use_duckdb_inference = bench_has_duckdb_infer(others)
    dir_queries = []

    for subdir in config["subdirs"]:
        subdir_queries = []
        for query_name in config["queries"]:
            query_name = str(query_name)
            user_specs = config.get("specs", [])
            if len(user_specs) == 0:
                subdir_queries.append(
                    Query(
                        query_name=query_name,
                        spec=QuerySpec(base="base.sql", key="base"),
                    ),
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

            with open(f"{parsed.layers}/{query_name}.config.json") as f:
                _config = json.loads(f.read())
                number_layers = len(_config)

            if not config.get("addTraceProv", True):
                continue

            extra_sync = [
                ExtraQuery(
                    label="traceprov_sync_time",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_0_sync_time.sql",
                    runs_after_base=True,
                    strict_run=True,
                )
            ]

            extra_drop_tables = [
                ExtraQuery(
                    label=f"traceprov_drop_table_{layer_id}",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_drop_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_drop_function = [
                ExtraQuery(
                    label=f"traceprov_drop_func_{layer_id}",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_drop_function.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_create_function = [
                ExtraQuery(
                    label=f"traceprov_create_func_{layer_id}",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_create_function.sql",
                    preprocess=[ReplaceFILE("traceprov_infer_set_path")],
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_create_table = [
                ExtraQuery(
                    label=(
                        f"traceprov_create_table_{layer_id}"
                        if is_validate
                        else f"traceprov_create_temp_table_{layer_id}"
                    ),
                    query=(
                        f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_create_table.sql"
                        if is_validate
                        else f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_create_temp_table.sql"
                    ),
                    runs_after_base=True,
                    skip_validation=True,
                    capture_output=False,
                )
                for layer_id in range(number_layers)
            ]

            extra_drop_tables_later = [
                ExtraQuery(
                    label=f"traceprov_drop_table_later_{layer_id}",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_drop_table.sql",
                    runs_after_base=True,
                    skip_validation=True,
                    strict_run=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_infer_time = [
                ExtraQuery(
                    label=f"traceprov_infer_time_{layer_id}",
                    query=f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_infer_time.sql",
                    runs_after_base=True,
                    strict_run=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_measure_count = [
                ExtraQuery(
                    label=f"traceprov_infer_count_{layer_id}",
                    query=f"$INLINE-select count(*) from layer_{layer_id};",
                    runs_after_base=True,
                    strict_run=True,
                    skip_validation=True,
                )
                for layer_id in range(number_layers)
            ]

            extra_measure_count = [
                ExtraQuery(
                    label=f"traceprov_infer_count_{layer_id}",
                    query=f"$INLINE-select count(*) from layer_{layer_id};",
                    runs_after_base=True,
                    strict_run=True,
                    skip_validation=True,
                )
                for layer_id in range(number_layers)
            ]

            extras = [
                *extra_sync,
                *extra_drop_tables,
                *extra_drop_function,
                *extra_create_function,
                *extra_create_table,
                *extra_infer_time,
                *extra_measure_count,
                *(extra_drop_tables_later if not is_validate else []),
            ]

            extra_labels = [e.label for e in extras]

            assert len(set(extra_labels)) == len(extra_labels)

            traceprov_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base="traceprov.sql",
                    key="traceprov",
                    extras=extras,
                ),
            )

            if parsed.traceprov:
                subdir_queries.append(traceprov_query)

            if is_validate:
                subdir_queries.append(
                    Query(
                        query_name=query_name,
                        spec=ValidationQuerySpec(
                            base="base.sql",
                            key="VALIDATION",
                            materialize="lineage_restricted.sql",
                        ),
                    ),
                )

            if has_sd:
                subdir_queries.append(
                    Query(
                        query_name=query_name,
                        spec=SmokedDuckQuerySpec(
                            base="base.sql",
                            key="SmokedDuck",
                        ),
                    )
                )
            if use_duckdb_inference:
                subdir_queries.append(
                    Query(
                        query_name=query_name,
                        spec=DuckDBInferenceQuerySpec(base="DUCKDB_INFERENCE"),
                    )
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


if __name__ == "__main__":
    main()
