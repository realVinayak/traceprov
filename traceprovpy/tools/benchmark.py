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
from traceprovpy.tools.run_with_timeout import (
    RunParams,
    run_with_timeout,
    RunWithTimeoutOptions,
)
from pathlib import Path
import os
import argparse


def _run_with_timeout(options: RunWithTimeoutOptions):
    print(options.file_path)
    return run_with_timeout(options)


# Special files that we should always skip.
SKIP_FILES = ["template_extract_gprom.sql"]


class ExtraQuery(NamedTuple):
    label: str
    query: str


class QuerySpec(NamedTuple):
    key: str
    base: str
    materialize: str | None = None
    extras: list[ExtraQuery] = []

    @staticmethod
    def get_pack(
        top_dir: Path,
        path: str,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
    ):
        query_path = top_dir.joinpath(path)
        assert query_path.exists(), "path not found: " + str(query_path)
        pack = get_run_options(query_path.as_posix())
        return pack

    def run_packs(
        self, top_dir: Path, get_run_options: Callable[[str], RunWithTimeoutOptions]
    ):
        base_pack = QuerySpec.get_pack(top_dir, self.base, get_run_options)
        materialize_pack = (
            QuerySpec.get_pack(top_dir, self.materialize, get_run_options)
            if self.materialize
            else None
        )
        results = dict(base=[], materialize=[], extras=[])

        for iter in range(base_pack.params.repeat + base_pack.params.throwaway):
            print("ON INDEX: ", iter)
            os.system(
                f'echo "select reinit_state();" | PGPASSWORD={base_pack.password} psql -U {base_pack.user} {base_pack.db}'
            )

            base_time = _run_with_timeout(base_pack)
            materialize_time = None
            assert (
                base_time is not None
            ), f"the base query should always execute: {base_pack}, {self}!"

            if materialize_pack:
                materialize_time = _run_with_timeout(materialize_pack)

            if iter < base_pack.params.throwaway:
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
        params=RunParams(repeat=4, throwaway=1),
    ):
        # Always run the analyze for statistics initially.
        os.system(f'echo "ANALYZE;" | PGPASSWORD={password} psql -U {user} {db_name}')
        print(directories)

        def _get_options(file_path: str):
            return RunWithTimeoutOptions(
                user=user,
                password=password,
                db=db_name,
                file_path=file_path,
                params=params,
            )

        results_from_dirs = {}
        for directory in directories:
            combined_results = {}
            for query in directory.queries:
                print(f"[{self.name}: ({directory.dir_name}, {query.query_name})]")
                results = query.spec.run_packs(
                    Path(top_dir) / directory.dir_name / query.query_name, _get_options
                )
                combined_results[query.query_name] = {
                    **combined_results.get(query.query_name, {}),
                    query.spec.key: results,
                }
            results_from_dirs[directory.dir_name] = combined_results

        return results_from_dirs
