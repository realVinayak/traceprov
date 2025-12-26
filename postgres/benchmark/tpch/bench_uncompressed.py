from traceprovpy.tools.benchmark import (
    DuckDbInferenceQuerySpec,
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    ValidationQuerySpec,
)
from traceprovpy.tools.benchmark_utils import TRACEPROV_DUMP_CSV
from traceprovpy.tools.perform_duckdb_inference import LayerSpec
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams
import json
import argparse


# This parses out the config file, and generates the directories
# on the fly, to be used.
# We log the directories later, anyways, so this is fine.
def main():
    benchmark = GenericBenchmark("tpch-driver")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-l", "--layers", required=True, type=str)
    parsed, _ = parser.parse_known_args()
    with open(parsed.config) as f:
        config: dict = json.loads(f.read())
    is_validate = config.get("validate", False)

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
                layer_config = json.loads(f.read())
                number_layers = len(layer_config)

            if not config.get("addTraceProv", True):
                continue

            # TODO: Renable this.
            if number_layers > 1:
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

            extras = [
                *extra_sync,
                *extra_drop_tables,
                *extra_drop_function,
                *extra_create_function,
                *extra_create_table,
                *extra_infer_time,
                *extra_measure_count,
                *(extra_drop_tables_later if not is_validate else []),
                TRACEPROV_DUMP_CSV(),
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

            layer_to_num_pk_map = {
                str(_layer_config["layer_number"]): len(_layer_config["pk_order"])
                for _layer_config in layer_config
            }
            layer_context = [
                LayerSpec(
                    layer_to_num_pk=layer_to_num_pk_map,
                    compression_scheme="uncompressed",
                    use_native=False,
                )
            ]
            subdir_queries.append(
                Query(
                    query_name=query_name,
                    spec=DuckDbInferenceQuerySpec(
                        base="DUCKDB_BASE",
                        key=f"DUCKDB_INFERENCE_{query_name}",
                        context=layer_context,
                    ),
                ),
            )

        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config["runTimeOptions"])
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config, layers=parsed.layers)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
