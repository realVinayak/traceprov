from traceprovpy.tools.benchmark import ExtraQuery, GenericBenchmark, Query, QueryDirectory, QuerySpec, ValidationQuerySpec
from traceprovpy.tools.run_with_timeout import ReplaceFILE, RunParams
import json
import argparse


# This parses out the config file, and generates the directories
# on the fly, to be used.
# We log the directories later, anyways, so this is fine.
def main():
    benchmark = GenericBenchmark("tpch-driver")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", '--config', required=True, type=str)
    parser.add_argument("-l", '--layers', required=True, type=str)
    parser.add_argument(
        "--validate", action=argparse.BooleanOptionalAction, default=False
    )
    parsed, _ = parser.parse_known_args()
    with open(parsed.config) as f:
        config = json.loads(f.read())

    dir_queries = []
    
    for subdir in config['subdirs']:
        subdir_queries = []
        for query_name in config['queries']:
            query_name = str(query_name)
            subdir_queries.append(
                Query(query_name=query_name, spec=QuerySpec(base="base.sql", key="base")),
            )

            with open(f"{parsed.layers}/{query_name}.config.json") as f:
                _config = json.loads(f.read())
                number_layers = len(_config)
            
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
                    label=(f"traceprov_create_table_{layer_id}" if parsed.validate else f"traceprov_create_temp_table_{layer_id}"),
                    query=(
                        f"$ROOT/../templates/layered/{query_name}/layer_{layer_id}_create_table.sql"
                        if parsed.validate
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

            traceprov_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base="traceprov.sql",
                    key="traceprov",
                    extras=[
                        *extra_sync,
                        *extra_drop_tables,
                        *extra_drop_function,
                        *extra_create_function,
                        *extra_create_table,
                        *(extra_drop_tables_later if not parsed.validate else []),
                        *extra_infer_time,
                    ],
                ),
            )

            subdir_queries.append(traceprov_query)
            if parsed.validate:
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
        
        dir_queries.append(
            QueryDirectory(dir_name=subdir, queries=subdir_queries)
        )
    
    benchmark.run_from_argparse(dir_queries, RunParams(repeat=1, throwaway=1))

if __name__ == "__main__":
    main()
