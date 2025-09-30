# Defines the general interface for a benchmark.
# The idea is that all the benchmarks follow some repeated pattern.
# Doing this also makes code usage better.


# The general expected folder structure is
# _Benchmark Name_
#   -- Params 1
#       -- Q01
#           -- base
#           -- gprom_window.sql
#           -- gprom_join.sql
#           -- traceprov.sql
#       -- Q02
#   -- Params 2
#       -- Q01
#       -- Q02

from typing import Callable, NamedTuple
from traceprovpy.tools.run_with_timeout import run_with_timeout, RunWithTimeoutOptions
from pathlib import Path
import os
import argparse

# Special files that we should always skip.
SKIP_FILES = ["template_extract_gprom.sql"]


class ExtraQuery(NamedTuple):
    label: str
    query: str


class QuerySpec(NamedTuple):
    key: str
    base: str
    materialize: str | None
    extras: list[ExtraQuery] = []

    @staticmethod
    def get_pack(
        top_dir: Path,
        path: str,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
    ):
        query_path = top_dir.joinpath(path)
        assert query_path.exists()
        pack = get_run_options(query_path.as_posix())
        return pack

    def run_packs(
        self, top_dir: Path, get_run_options: Callable[[str], RunWithTimeoutOptions]
    ):
        base_pack = QuerySpec.get_pack(top_dir, self.base, get_run_options)
        materialize_pack = (
            QuerySpec.get_pack(self.materialize) if self.materialize else None
        )
        results = dict(base=[], materialize=[], extras=[])

        for iter in range(base_pack.repeat + base_pack.throwaway):
            print("ON INDEX: ", iter)
            os.system(
                f'echo "select reinit_state();" | PGPASSWORD={base_pack.password} psql -U {base_pack.user} {base_pack.db}'
            )

            base_time = run_with_timeout(base_pack)
            materialize_time = None
            assert (
                base_time is not None
            ), f"the base query should always execute: {base_pack}, {self}!"

            if materialize_pack:
                materialize_time = run_with_timeout(materialize_pack)

            if iter < base_pack.throwaway:
                continue
            results["base"].append(base_time)
            if materialize_time:
                results["materialize"].append(materialize_time)

        return results


class Query(NamedTuple):
    query_name: str
    spec: QuerySpec


class QueryDirectory(NamedTuple):
    dir_name: str
    queries: list[Query]


class GenericBenchmark(NamedTuple):
    name: str

    def run(
        self,
        user: str,
        password: str,
        db_name: str,
        top_dir: str,
        directories: list[QueryDirectory],
    ):

        def _get_options(file_path: str):
            return RunWithTimeoutOptions(
                user=user, password=password, db=db_name, file_path=file_path
            )

        results_from_dirs = {}
        for directory in directories:
            combined_results = {}
            for query in directory.queries:
                print(f"[{self.name}: ({directory.dir_name}, {query.query_name})]")
                results = query.spec.run_packs(top_dir, _get_options)
                assert query.query_name not in combined_results
                combined_results[query.query_name] = results
            results_from_dirs[directory.dir_name] = combined_results

        return results_from_dirs
