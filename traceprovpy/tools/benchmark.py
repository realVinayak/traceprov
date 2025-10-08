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

from typing import Callable, Literal, NamedTuple
from traceprovpy.tools.run_with_timeout import (
    ConnectionParams,
    RunParams,
    run_with_timeout,
    RunWithTimeoutOptions,
)
from pathlib import Path
import os
import argparse
import time


def _run_with_timeout(options: RunWithTimeoutOptions):
    print(options.file_path)
    return run_with_timeout(options)


class ExtraQuery(NamedTuple):
    label: str
    query: str
    # We allow some arbitrary queries to run, right after a previous query.
    # The depends_on helps control when this query runs.
    # For example, we'd run
    should_run: Callable[["QuerySpec", Literal["base", "materialize"]], bool]
    skip_validation: bool = False


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
        if path.startswith("$ROOT"):
            query_path = Path(path.replace("$ROOT", os.getcwd()))
        else:
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

        def _run_extras(extras: list[ExtraQuery], _extra_context: dict | None = None):
            extra_results = dict()
            for extra in extras:
                extra_pack = QuerySpec.get_pack(
                    top_dir, extra.query, get_run_options
                )._replace(
                    capture_output=True,
                    extras=_extra_context,
                    skip_validation=extra.skip_validation,
                )
                extra_result = _run_with_timeout(extra_pack)
                extra_results[extra.label] = extra_result
            return extra_results

        iter_count = 0
        start_perf_counter = time.perf_counter()
        while True:
            # for iter in range(base_pack.params.repeat + base_pack.params.throwaway):
            end_perf_counter = time.perf_counter()
            if base_pack.params.execution_time is not None:
                if base_pack.params.execution_time <= (
                    end_perf_counter - start_perf_counter
                ):
                    break
            if base_pack.params.repeat is not None:
                if iter_count >= base_pack.params.repeat + base_pack.params.throwaway:
                    break

            print("ON INDEX: ", iter_count)
            flat_options = base_pack.connection_params.get_flat()
            os.system(
                f'echo "select reinit_state();" | PGPASSWORD={base_pack.connection_params.password} psql {flat_options}'
            )

            base_time = _run_with_timeout(base_pack)
            materialize_time = None
            assert (
                base_time is not None
            ), f"the base query should always execute: {base_pack}, {self}!"

            base_extras_to_run = [
                extra for extra in self.extras if extra.should_run(self, "base")
            ]

            materialize_context = dict()
            if materialize_pack:
                materialize_pack = materialize_pack._replace(extras=materialize_context)
                materialize_time = _run_with_timeout(materialize_pack)
                assert len(materialize_context) != 0

            if (
                base_pack.params.throwaway is not None
                and iter_count < base_pack.params.throwaway
            ):
                iter_count += 1
                continue
            results["base"].append(base_time)

            base_extra_results = _run_extras(base_extras_to_run)
            materialize_extra_results = {}

            if materialize_time:
                results["materialize"].append(materialize_time)
                mat_extras_to_run = [
                    extra
                    for extra in self.extras
                    if extra.should_run(self, "materialize")
                ]
                materialize_extra_results = _run_extras(
                    mat_extras_to_run, materialize_context
                )

            # This is where we end up closing the connections.
            if materialize_pack:
                materialize_pack.close_all()

            merged_extra_results = {
                **base_extra_results,
                **materialize_extra_results,
            }
            assert len(base_extra_results) + len(materialize_extra_results) == len(
                merged_extra_results
            )
            if len(merged_extra_results) > 0:
                results["extras"].append(merged_extra_results)

            iter_count += 1

        return results


class Query(NamedTuple):
    query_name: str
    spec: QuerySpec


class QueryDirectory(NamedTuple):
    dir_name: str
    queries: list[Query]


class GenericBenchmark(NamedTuple):
    name: str

    def run_from_argparse(
        self,
        directories: list[QueryDirectory],
        params=RunParams(),
    ):
        parser = argparse.ArgumentParser(prog=f"run-{self.name}")
        parser.add_argument("-u", "--user", required=True)
        parser.add_argument("-p", "--password", required=True)
        parser.add_argument("-H", "--host", required=False, default="127.0.0.1")
        parser.add_argument("-P", "--port", required=False, default="5432")
        parser.add_argument("-db", "--db", required=True)
        parser.add_argument("-suff", "--suff", required=True)
        parser.add_argument("-tp_root", "--traceprov_root", required=True)
        parser.add_argument("-t_root", "--test_root", required=True)

        parsed = parser.parse_args()
        connection_params = ConnectionParams(
            host=parsed.host,
            port=parsed.port,
            user=parsed.user,
            password=parsed.port,
            database=parsed.db,
        )
        self.setup(parsed.traceprov_root, connection_params, parsed.suff)

        start = time.perf_counter()
        result = self.run(
            parsed.test_root,
            directories,
            connection_params,
            params,
        )
        end = time.perf_counter()
        final_result = dict(result=result, time_taken=end - start, db=parsed.db)
        return final_result

    def setup(
        self,
        traceprov_postgres_root: str,
        connection_params: ConnectionParams,
        suff: str = None,
    ):
        if suff is None:
            suff = self.name
        os.system(
            f"cd {traceprov_postgres_root} && make clean && make traceprov suff={suff} && make infer_set suff={suff}"
        )
        traceprov_sql = Path(traceprov_postgres_root) / f"traceprov_{suff}.auto.sql"
        traceprov_infer_set = (
            Path(traceprov_postgres_root) / f"traceprov_return_infer_{suff}.auto.sql"
        )
        assert traceprov_sql.exists(), f"{traceprov_sql.as_posix()} should exist!"
        assert (
            traceprov_infer_set.exists()
        ), f"{traceprov_infer_set.as_posix()} should exist!"

        os.system(
            f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_sql.as_posix()}"
        )

        os.system(
            f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_infer_set.as_posix()}"
        )

    def run(
        self,
        top_dir: str,
        directories: list[QueryDirectory],
        connection_params: ConnectionParams,
        params=RunParams(),
    ):
        # Always run the analyze for statistics initially.
        os.system(
            f'echo "ANALYZE;" | PGPASSWORD={connection_params.password} psql {connection_params.get_flat()}'
        )
        print(directories)
        params.validate()

        def _get_options(file_path: str):
            return RunWithTimeoutOptions(
                connection_params=connection_params,
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
