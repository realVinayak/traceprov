from argparse import ArgumentParser
import argparse
import json
from pathlib import Path
from typing import Callable, NamedTuple, Set, Tuple
from traceprovpy.tools.benchmark_utils import TRACEPROV_SQL_DERIVATION_QUERY
from traceprovpy.tools.convert_csv_to_duckdb import convert_csv_to_duckdb
from traceprovpy.tools.benchmark import GenericBenchmark, QuerySpec
from traceprovpy.tools.file_utils import just_write, traceprov_assert_safe_run
from traceprovpy.tools.run_with_timeout import (
    DEFAULT_REPEAT,
    DEFAULT_THROWAWAY,
    TP_SKIPPABLE_OPTION,
    RunWithTimeoutOptions,
    run_with_timeout,
)
from traceprovpy.tools.copy_traceprov_dump import make_copy
import os

TRACEPROV_GRAPH_FILE = "/tmp/traceprov/graph.bin"


class DriverDefaultValues(NamedTuple):
    traceprov_use_partition_in_agg: bool = False
    traceprov_skip_page_cache: bool = False
    traceprov_use_merge_chunks: bool = False
    traceprov_use_implicit_union: bool = True
    traceprov_use_compact: bool = False
    traceprov_force_seq_scan: bool = False
    traceprov_skip_sql_cache: bool = False
    traceprov_use_table_stats: bool = False
    traceprov_ignore_direct_join: bool = False


DriverDefaultValuesInstance = DriverDefaultValues()


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
    log_offset: int | None = None
    traceprov_perform_derivation: bool = False
    traceprov_materialize_derivation: bool = False
    traceprov_dry_run_derivation: bool = False
    traceprov_layers_to_derive: Tuple[int] | None = None
    extras: list[str] = None
    extra_files: list[str] = None
    log_offsets: list[str] = None
    pre_query: list[str] = None
    extra_multiple_count: int | None = 3
    get_log_size: bool = False
    # All optimizations.
    traceprov_use_partition_in_agg: bool = (
        DriverDefaultValues.traceprov_use_partition_in_agg
    )
    traceprov_skip_page_cache: bool = DriverDefaultValues.traceprov_skip_page_cache
    traceprov_use_merge_chunks: bool = DriverDefaultValues.traceprov_use_merge_chunks
    traceprov_use_implicit_union: bool = (
        DriverDefaultValues.traceprov_use_implicit_union
    )
    traceprov_use_compact: bool = DriverDefaultValues.traceprov_use_compact
    traceprov_force_seq_scan: bool = DriverDefaultValues.traceprov_force_seq_scan
    traceprov_skip_sql_cache: bool = DriverDefaultValues.traceprov_skip_sql_cache
    traceprov_use_table_stats: bool = DriverDefaultValues.traceprov_use_table_stats
    traceprov_ignore_direct_join: bool = (
        DriverDefaultValues.traceprov_ignore_direct_join
    )
    warm_up_time: int = 0
    load_micro_benchmarks: bool = False
    sd_join_mode: bool = False

    @staticmethod
    def get_suffix(parsed):
        optimizations = DuckDBDriverOptions._optimizations()
        # a gentler mapping.
        mapped = {
            opt: opt.replace("traceprov_use_", "").replace("traceprov_", "")
            for opt in optimizations
        }
        assert len(mapped) == len(optimizations)
        mapped = {**mapped, "threads": "threads", "optimized": "optimized"}
        value_mapping = sorted(
            [(key, value, getattr(parsed, key)) for (key, value) in mapped.items()],
            key=lambda x: x[0],
        )
        suffix = []
        for key, nice_label, value in value_mapping:
            final_value = value
            if isinstance(value, bool):
                final_value = "y" if value else "n"
                # if the value is the default value, don't bother.
                if value == getattr(DriverDefaultValuesInstance, key, None):
                    continue
            suffix.append(f"{nice_label}-{final_value}")
        print(suffix)
        return "__".join(suffix)

    @staticmethod
    def _optimizations():
        optimizations = {
            "traceprov_use_partition_in_agg",
            "traceprov_skip_page_cache",
            "traceprov_use_merge_chunks",
            "traceprov_use_implicit_union",
            "traceprov_use_compact",
            "traceprov_force_seq_scan",
            "traceprov_skip_sql_cache",
            "traceprov_use_table_stats",
            "traceprov_ignore_direct_join",
        }
        recognized = set(DriverDefaultValuesInstance._fields)
        assert recognized == optimizations
        return optimizations

    def _boolean_options(self):
        optimizations = DuckDBDriverOptions._optimizations()
        base_options = {
            "lineage",
            "pending",
            "no_reinit",
            "disable_col_opt",
            "main_once_extra_all",
            "is_new_sd",
            "traceprov_perform_derivation",
            "traceprov_materialize_derivation",
            "traceprov_dry_run_derivation",
            "get_log_size",
            "load_micro_benchmarks",
            "sd_join_mode",
        }
        assert (
            len(optimizations.intersection(base_options)) == 0
        ), "Expected no common ones"
        return base_options | optimizations

    @staticmethod
    def add_parse_options(parser: ArgumentParser):
        optimizations = DuckDBDriverOptions._optimizations()
        misc_bool_options = {"pending", "traceprov_dry_run_derivation"}
        for optimization in optimizations | misc_bool_options:
            parser.add_argument(
                f"--{optimization}",
                action=argparse.BooleanOptionalAction,
                default=(
                    getattr(DriverDefaultValuesInstance, optimization)
                    if optimization in optimizations
                    else False
                ),
            )

        parser.add_argument("--threads", type=int, default=1)
        parser.add_argument("--db", required=True)
        print("defaults:: ", DuckDBDriverOptions._field_defaults)
        parser.add_argument(
            "--extra_multiple_count",
            type=int,
            default=DuckDBDriverOptions._field_defaults["extra_multiple_count"],
        )
        parser.add_argument("--warm_up_time", type=int, default=0)

    def parse_optimizations(self, parsed):
        optimizations = self._optimizations()
        kwargs = {
            optimization: getattr(parsed, optimization)
            for optimization in optimizations
        }
        return self._replace(**kwargs)

    def get_list_options(self) -> list[str]:
        options = self._asdict()
        boolean_options = [
            f"--{key}" for key in self._boolean_options() if options[key] == True
        ]
        key_value_options = [
            f"--{key} {value}"
            for (key, value) in options.items()
            if (key not in self._boolean_options() and value is not None)
            and (key not in ["extra", "extras", "extra_files"])
            and (key not in ["traceprov_layers_to_derive"])
            and (key not in ["log_offsets"])
            and (key not in ["pre_query"])
        ]
        all_extras = [*([self.extra] if self.extra else []), *(self.extras or [])]
        key_value_options = [*key_value_options, *[f"--extra {f}" for f in all_extras]]
        key_value_options = [
            *key_value_options,
            *[f"--extra_file {file}" for file in self.extra_files or []],
        ]
        if self.traceprov_layers_to_derive:
            key_value_options = [
                *key_value_options,
                *[
                    f"--traceprov_layers_to_derive {layer_to_derive}"
                    for layer_to_derive in self.traceprov_layers_to_derive
                ],
            ]
        if self.log_offsets:
            key_value_options = [
                *key_value_options,
                *[f"--log_offset {log_offset}" for log_offset in self.log_offsets],
            ]
        if self.pre_query:
            pre_query_file = "/tmp/traceprov_pre_query.txt"
            just_write(pre_query_file, "\n".join(self.pre_query))
            key_value_options = [
                *key_value_options,
                *[f"--pre_main_sql {pre_query_file}"],
            ]

        print(key_value_options)
        assert self.i is not None
        with open(self.i) as f:
            contents = f.read()
        assert len(contents) > 0, f"Got no contents for {self.i}"
        return [*boolean_options, *key_value_options]

    def serialize(self) -> str:
        return " ".join(self.get_list_options())


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
        destination_dir: Path = self.extra_options["destination_dir"]
        make_copy(pg_pack.connection_params, destination_dir.as_posix())


class ExtractJsonGraphQuerySpec(QuerySpec):
    def run_packs(self, top_dir, get_run_options, benchmark):
        pg_pack = get_run_options(TP_SKIPPABLE_OPTION)
        assert self.extra_options is not None
        source_path: Path = self.extra_options["source_graph_path"]
        target_path: Path = self.extra_options["target_graph_path"]
        make_copy(pg_pack.connection_params, source_path.as_posix(), True)
        query_file = just_write(
            "/tmp/traceprov_json_query.sql", "select * from traceprov_json_graph();"
        )
        pg_pack = pg_pack._replace(
            strict_run=True,
            capture_output=True,
            file_path=query_file,
            skip_validation=True,
            shared_libraries=[
                *pg_pack.shared_libraries,
                benchmark.traceprov_path,
                benchmark.traceprov_infer_set_path,
            ],
        )
        source_graph = json.loads(run_with_timeout(pg_pack)["captured"][0][0])
        make_copy(pg_pack.connection_params, target_path.as_posix(), True)
        target_graph = json.loads(run_with_timeout(pg_pack)["captured"][0][0])
        print(source_path, "Same: ", source_graph == target_graph)
        return dict(
            source_graph=source_graph,
            target_graph=target_graph,
            same=source_graph == target_graph,
        )
