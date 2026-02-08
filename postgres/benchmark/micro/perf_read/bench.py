import argparse
from itertools import product
import json
from pathlib import Path
from typing import Callable, List

from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions, run_simple_query
from traceprovpy.tools.run_with_timeout import (
    DEFAULT_REPEAT,
    DEFAULT_THROWAWAY,
    ReplaceSelectivity,
    RunParams,
    RunWithTimeoutOptions,
)


total = DEFAULT_REPEAT + DEFAULT_THROWAWAY


class DuckDBSpec(QuerySpec):
    def run_packs(
        self,
        top_dir: Path,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
        benchmark: GenericBenchmark,
    ):
        sd_options = benchmark.sd_options
        assert sd_options is not None, "Expected duckdb options to be set!"
        executable = benchmark.sd_options.driver_executable

        num_entries = self.extra_options.get("num_elements")
        width = self.extra_options.get("width")

        destination_dir = top_dir / "tmp/duckdb_inference/"

        traceprov_assert_safe_run(f"rm -rf {destination_dir.as_posix()}")
        traceprov_assert_safe_run(f"mkdir -p {destination_dir.as_posix()}")
        duckdb_db_dir = destination_dir / "duck_inference.db"
        run_simple_query(
            f"create table entries_{num_entries} as (select {dump_generate_series(width)} from generate_series(1, {num_entries}));",
            executable,
            duckdb_db_dir.as_posix(),
            repeat=1,
        )
        with open("/tmp/duckdb_read.sql", "w") as f:
            f.write(f"select * from entries_{num_entries};")

        driver_options = DuckDBDriverOptions(
            db=duckdb_db_dir.as_posix(),
            i="/tmp/duckdb_read.sql",
            time="/tmp/duckdb_result.json",
            threads=sd_options.number_of_threads,
            settings="/tmp/duckdb_stats.json",
            repeat=total,
        )
        traceprov_assert_safe_run(f"{executable} {driver_options.serialize()}")
        with open(driver_options.time) as f:
            duckdb_result_spec = json.loads(f.read())
        return dict(duckdb_result_spec=duckdb_result_spec)


def dump_generate_series(width: int):
    return ",".join(["generate_series::bigint"] * width)


def make_simple_query(num_elements: int, width: int):

    replaces_selectivity = ReplaceSelectivity(num_elements)
    replaces_clause = ReplaceSelectivity(dump_generate_series(width), ":clauses")

    base_query = Query(
        query_name=f"create_log",
        spec=QuerySpec(
            base="create.sql",
            key=f"create_log_entries_{num_elements}_{width}",
            preprocess=[replaces_selectivity, replaces_clause],
            extras=[
                ExtraQuery(
                    label="traceprov_read_log",
                    query=f"$INLINE-select * from traceprov_perf_read(1, {total});",
                    runs_after_base=True,
                    strict_run=True,
                    capture_output=True,
                )
            ],
        ),
    )
    return base_query


def duckdb_query(num_elements: int, width: int):
    return Query(
        query_name=f"create_log",
        spec=DuckDBSpec(
            base="DUCKDB_READ",
            key=f"DUCKDB_INFERENCE_{num_elements}_{width}",
            extra_options=dict(num_elements=num_elements, width=width),
        ),
    )


def make_directory(scales: List[int], widths: List[int]):
    args = list(product(scales, widths))
    directory = QueryDirectory(
        dir_name="read_query",
        queries=[
            *[make_simple_query(*arg) for arg in args],
            # *[duckdb_query(*arg) for arg in args],
        ],
    )
    return directory


directores = []


def main():
    benchmark = GenericBenchmark("perf_read")
    scales = [10_000, 1_000_000, 5_000_000, 10_000_000]
    widths = [1]
    directory = make_directory(scales, widths)
    result = benchmark.run_from_argparse(
        [directory], params=RunParams(repeat=1, throwaway=0)
    )
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
