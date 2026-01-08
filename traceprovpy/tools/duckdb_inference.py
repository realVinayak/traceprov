import json
from pathlib import Path
from typing import Callable, NamedTuple, Set
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_SQL_DERIVATION_QUERY,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.convert_csv_to_duckdb import convert_csv_to_duckdb
from traceprovpy.tools.benchmark import GenericBenchmark, QuerySpec
from traceprovpy.tools.run_with_timeout import (
    DEFAULT_REPEAT,
    TP_SKIPPABLE_OPTION,
    RunWithTimeoutOptions,
)
from traceprovpy.tools.copy_traceprov_dump import make_copy
import os


class DuckDBDriverOptions(NamedTuple):
    db: str
    i: str
    lineage: bool = None
    profile: str = None
    pending: bool = None
    threads: int = None
    stats: str = None
    repeat: int = DEFAULT_REPEAT
    settings: str = None
    time: str = None

    def _boolean_options(self):
        return {"lineage", "pending"}

    def serialize(self) -> str:
        options = self._asdict()
        boolean_options = [
            f"--{key}" for key in self._boolean_options() if options[key] == True
        ]
        key_value_options = [
            f"--{key} {value}"
            for (key, value) in options.items()
            if (key not in self._boolean_options() and value is not None)
        ]
        return " ".join([*boolean_options, *key_value_options])


def run_simple_query(query: str, db_executable: str, db_path: str, **options):
    tmp_query = "/tmp/duckdb_simple_query.sql"
    with open(tmp_query, "w") as f:
        f.write(query)
    materialize_options = DuckDBDriverOptions(db=db_path, i=tmp_query, **options)
    traceprov_assert_safe_run(f"{db_executable} {materialize_options.serialize()}")


# def driver_run_query(executable: str, num_threads: int, num_repeat: int, )
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
        traceprov_assert_safe_run(f"sudo rm -rf {destination_dir.as_posix()}")
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
            executable = benchmark.sd_options.driver_executable
            assert executable is not None
            print(executable)
            duckdb_inference_sql_query = "/tmp/duckdb_inference_dynamic.sql"
            with open(duckdb_inference_sql_query, "w") as f:
                f.write(sql_query)
            driver_options = DuckDBDriverOptions(
                db=duckdb_db_dir.as_posix(),
                i=duckdb_inference_sql_query,
                time="/tmp/duckdb_result.json",
            )
            traceprov_assert_safe_run(f"{executable} {driver_options.serialize()}")
            with open("/tmp/duckdb_result.json") as f:
                duckdb_result_spec = json.loads(f.read())
                print(duckdb_result_spec)

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
            final_results = [
                *final_results,
                {**result_spec, "duckdb_inference": duckdb_result_spec},
            ]
        return final_results
