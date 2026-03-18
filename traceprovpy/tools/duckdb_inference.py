import json
from pathlib import Path
from typing import Callable, NamedTuple, Set, Tuple
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_SQL_DERIVATION_QUERY,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.convert_csv_to_duckdb import convert_csv_to_duckdb
from traceprovpy.tools.benchmark import GenericBenchmark, QuerySpec
from traceprovpy.tools.run_with_timeout import (
    DEFAULT_REPEAT,
    DEFAULT_THROWAWAY,
    TP_SKIPPABLE_OPTION,
    RunWithTimeoutOptions,
)
from traceprovpy.tools.copy_traceprov_dump import make_copy
import os

TRACEPROV_GRAPH_FILE = "/tmp/traceprov/graph.bin"

# TODO: Breakk this up.
# TODO: Add automatic parse arguments. Cmon that already exists in other places.
# TODO: Add separate options for the settings (slightly complicated to do automatically, but better than current)
class DuckDBDriverOptions(NamedTuple):
    db: str
    i: str
    lineage: bool | None = None
    profile: str | None = None
    pending: bool | None = None
    threads: int | None = None
    stats: str | None = None
    repeat: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY
    settings: str | None = None
    time: str | None = None
    idx_scan_percent: str | None = None
    no_reinit: bool | None = None
    min_layer_number: None | int = None
    extra: str | None = None
    disable_col_opt: bool | None = None
    main_once_extra_all: bool = False
    extra_file: str | None = None
    is_new_sd: bool = False
    sd_extension_path: str | None = None
    traceprov_use_partition_in_agg: bool = False
    traceprov_use_partition_in_log: bool = False
    traceprov_use_row_in_agg_partition: bool = False
    top_log_num: int | None = None
    log_offset: int | None = None
    use_part_agg: list[int] | None = None
    traceprov_perform_derivation: bool = False
    traceprov_materialize_derivation: bool = False
    traceprov_skip_page_cache: bool = False
    traceprov_use_implicit_union: bool = False
    traceprov_dry_run_derivation: bool = False
    traceprov_layers_to_derive: Tuple[int] | None = None
    traceprov_use_merge_chunks: bool = False
    traceprov_combine_in_memory: bool = False
    extras: list[str] = None
    traceprov_split_combine: bool = False
    extra_files: list[str] = None
    use_extra_threads: bool = False
    traceprov_use_compact: bool = False

    def set_part_agg(self, part_agg: int):
        new_list = self.use_part_agg or []
        return self._replace(use_part_agg=[*new_list, part_agg])

    def _boolean_options(self):
        return {
            "lineage",
            "pending",
            "no_reinit",
            "disable_col_opt",
            "main_once_extra_all",
            "is_new_sd",
            "traceprov_use_partition_in_agg",
            "traceprov_use_partition_in_log",
            "traceprov_use_row_in_agg_partition",
            "traceprov_perform_derivation",
            "traceprov_materialize_derivation",
            "traceprov_skip_page_cache",
            "traceprov_use_implicit_union",
            "traceprov_dry_run_derivation",
            "traceprov_use_merge_chunks",
            "traceprov_combine_in_memory",
            "traceprov_split_combine",
            "use_extra_threads",
            "traceprov_use_compact"
        }

    def serialize(self) -> str:
        options = self._asdict()
        boolean_options = [
            f"--{key}" for key in self._boolean_options() if options[key] == True
        ]
        key_value_options = [
            f"--{key} {value}"
            for (key, value) in options.items()
            if (key not in self._boolean_options() and value is not None)
            and (key not in ["extra", "extras", "use_part_agg", "extra_files"])
            and (key not in ["traceprov_layers_to_derive"])
        ]
        all_extras = [*([self.extra] if self.extra else []), *(self.extras or [])]
        key_value_options = [*key_value_options, *[f"--extra {f}" for f in all_extras]]
        key_value_options = [
            *key_value_options,
            *[f"--use_part_agg {f}" for f in self.use_part_agg or []],
        ]
        key_value_options = [
            *key_value_options,
            *[f'--extra_file {file}' for file in self.extra_files or []]
        ]
        if self.traceprov_layers_to_derive:
            key_value_options = [
                *key_value_options, 
                *[f"--traceprov_layers_to_derive {layer_to_derive}" for layer_to_derive in self.traceprov_layers_to_derive]
            ]

        print(key_value_options)
        assert self.i is not None
        with open(self.i) as f:
            contents = f.read()
        assert len(contents) > 0, f"Got no contents for {self.i}"
        return " ".join([*boolean_options, *key_value_options])


def run_simple_query(query: str, db_executable: str, db_path: str, **options):
    tmp_query = "/tmp/duckdb_simple_query.sql"
    with open(tmp_query, "w") as f:
        f.write(query)
    materialize_options = DuckDBDriverOptions(db=db_path, i=tmp_query, **options)
    traceprov_assert_safe_run(f"{db_executable} {materialize_options.serialize()}")


# Note that we end up utilzing the duckdb driver
# which also gets used for smokedduck.
class DuckDBInferenceQuerySpec(QuerySpec):
    def run_packs(
        self,
        top_dir: Path,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
        benchmark: GenericBenchmark,
    ):
        sd_options = benchmark.sd_options
        assert sd_options is not None, "Expected duckdb options to be set!"

        pg_pack = get_run_options(TP_SKIPPABLE_OPTION)

        # Get the SQL for traceprov inference
        connection = pg_pack.connection_params.make_connection()
        cursor = connection.cursor()
        cursor.execute(TRACEPROV_SQL_DERIVATION_QUERY)
        result = cursor.fetchall()
        result_specs = json.loads(result[0][0])

        destination_dir = top_dir / "tmp/duckdb_inference/"
        traceprov_assert_safe_run(f"rm -rf {destination_dir.as_posix()}")
        make_copy(pg_pack.connection_params, destination_dir.as_posix())

        duckdb_db_dir = destination_dir / "duck_inference.db"

        created_tables = set()
        # create duckdb tables from the CSV.
        for root, _, files in os.walk(destination_dir):
            for file in files:
                if file.endswith(".csv"):
                    created_table = convert_csv_to_duckdb(
                        os.path.join(root, file), duckdb_db_dir.as_posix(), True
                    )
                    assert not created_table in created_tables
                    created_tables.add(created_table)

        final_results = []
        for result_spec in result_specs:
            print(result_spec)
            layer_idx = result_spec["idx"]

            sql_query = result_spec["sql"]
            executable = sd_options.driver_executable
            assert executable is not None
            print(executable)
            duckdb_inference_sql_query = "/tmp/duckdb_inference_dynamic.sql"
            with open(duckdb_inference_sql_query, "w") as f:
                f.write(sql_query)
            create_idx_queries = result_spec["create_idx"]
            idx_scan_percent = None
            if create_idx_queries and sd_options.create_idx:
                for create_idx_query in create_idx_queries.split(";"):
                    print(create_idx_query)
                    run_simple_query(
                        create_idx_query,
                        executable,
                        duckdb_db_dir.as_posix(),
                        repeat=1,
                    )
                idx_scan_percent = "1"

            driver_options = DuckDBDriverOptions(
                db=duckdb_db_dir.as_posix(),
                i=duckdb_inference_sql_query,
                time="/tmp/duckdb_result.json",
                threads=sd_options.number_of_threads,
                settings="/tmp/duckdb_stats.json",
                idx_scan_percent=idx_scan_percent,
            )
            traceprov_assert_safe_run(f"{executable} {driver_options.serialize()}")
            assert driver_options.time and driver_options.settings
            with open(driver_options.time) as f:
                duckdb_result_spec = json.loads(f.read())
                # print(duckdb_result_spec)

            with open(driver_options.settings) as f:
                duckdb_settings = json.loads(f.read())
                # print(duckdb_settings)

            run_simple_query(
                f"COPY (select * from ({sql_query}) F ORDER BY ALL) to 'duckdb_inference_{layer_idx}.tmp' (FORMAT CSV, HEADER false);",
                executable,
                duckdb_db_dir.as_posix(),
                repeat=1,
            )

            final_dump_name = f"final_output_{layer_idx}_dump"
            assert final_dump_name in created_tables

            run_simple_query(
                f"COPY (select * from {final_dump_name} F ORDER BY ALL) to 'traceprov_inference_{layer_idx}.tmp' (FORMAT CSV, HEADER false);",
                executable,
                duckdb_db_dir.as_posix(),
                repeat=1,
            )

            traceprov_assert_safe_run(
                f"diff duckdb_inference_{layer_idx}.tmp traceprov_inference_{layer_idx}.tmp"
            )
            driver_options_profiled = driver_options._replace(
                profile="/tmp/duckdb_profile.json", repeat=1
            )
            traceprov_assert_safe_run(
                f"{executable} {driver_options_profiled.serialize()}"
            )
            assert driver_options_profiled.profile
            with open(driver_options_profiled.profile) as f:
                duckdb_terminal_profile = json.loads(f.read())
            final_results = [
                *final_results,
                {
                    **result_spec,
                    "duckdb_inference": duckdb_result_spec,
                    "duckdb_settings": duckdb_settings,
                    "duckdb_terminal_profile": duckdb_terminal_profile,
                },
            ]
        return final_results


# copies the graph.bin file.
class DuckDbInferenceBinQuerySpec(QuerySpec):
    def run_packs(
        self,
        top_dir: Path,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
        benchmark: GenericBenchmark,
    ):
        pg_pack = get_run_options(TP_SKIPPABLE_OPTION)
        assert self.extra_options is not None
        destination_dir : Path = self.extra_options['destination_dir']
        make_copy(pg_pack.connection_params, destination_dir.as_posix())