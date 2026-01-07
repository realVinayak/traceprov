import json
from pathlib import Path
from typing import Callable
from traceprovpy.tools.benchmark_utils import TRACEPROV_SQL_DERIVATION_QUERY, traceprov_assert_safe_run
from traceprovpy.tools.convert_csv_to_duckdb import convert_csv_to_duckdb
from traceprovpy.tools.benchmark import GenericBenchmark, QuerySpec
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, RunWithTimeoutOptions
from traceprovpy.tools.copy_traceprov_dump import make_copy
import os
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
        destination_dir = top_dir / "tmp/duckdb_inference/"
        traceprov_assert_safe_run(f"sudo rm -rf {destination_dir.as_posix()}")
        make_copy(pg_pack, destination_dir.as_posix())

        duckdb_db_dir = destination_dir / "duck_inference.db"

        # create duckdb tables from the CSV.        
        for root, _, files in os.walk(destination_dir):
            for file in files:
                if file.endswith(".csv"):
                    convert_csv_to_duckdb(os.path.join(root, file), duckdb_db_dir.as_posix(), True)

        # Get the SQL for traceprov inference
        connection = pg_pack.connection_params.make_simple_connection()
        cursor = connection.cursor()
        cursor.execute(TRACEPROV_SQL_DERIVATION_QUERY)
        result = cursor.fetchall()
        result_spec = json.loads(cursor[0][0])
        
        
