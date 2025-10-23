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

from typing import Any, Callable, NamedTuple, Tuple
from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.run_with_timeout import (
    ConnectionParams,
    Preprocessor,
    ReplaceFILE,
    ReplaceSelectivity,
    RunParams,
    run_with_timeout,
    RunWithTimeoutOptions,
)
from pathlib import Path, PosixPath
import os
import argparse
import time
import json
from datetime import date, datetime

from traceprovpy.tools.stats.stats_collector import StatsCollector
import decimal


def json_serial(obj):
    # We've some datetimes that aren't natively json serializable. So, we have this wrapper.
    # Adapted from stack overflow: https://stackoverflow.com/a/22238613.

    if isinstance(obj, (datetime, date)):
        return obj.isoformat()

    if isinstance(obj, ReplaceFILE):
        return dict(
            preprocessor_type=ReplaceFILE.__name__, param=obj.replace_with_token
        )

    if isinstance(obj, ReplaceSelectivity):
        return dict(
            preprocessor_type=ReplaceSelectivity.__name__, param=obj.selectivity
        )

    if isinstance(obj, PosixPath):
        return obj.as_posix()

    if isinstance(obj, decimal.Decimal):
        return str(obj)

    raise TypeError("Type %s not serializable" % type(obj))


def _run_with_timeout(options: RunWithTimeoutOptions):
    print(options.file_path)
    return run_with_timeout(options)


class ExtraQuery(NamedTuple):
    label: str
    query: str
    # We allow some arbitrary queries to run, right after a previous query.
    runs_after_base: bool = False
    runs_after_materialize: bool = False
    skip_validation: bool = False
    preprocess: list[Preprocessor] = []
    capture_output: bool = True
    strict_run: bool = False


class QuerySpec(NamedTuple):
    key: str
    base: str
    materialize: str | None = None
    extras: list[ExtraQuery] = []
    preprocess: list[Preprocessor] = []

    @staticmethod
    def get_pack(
        top_dir: Path,
        path: str,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
    ):
        print(path)
        if path.startswith("$ROOT"):
            query_path = Path(path.replace("$ROOT", os.getcwd()))
        elif path.startswith("$INLINE-"):
            # We allow passing queries inline (especially when it is convenient)
            sql_query = path.replace("$INLINE-", "")
            query_path = Path("/tmp/traceprov_inline_query.sql")
            with open(query_path, "w") as f:
                f.write(sql_query)
        else:
            query_path = top_dir.joinpath(path)
        assert query_path.exists(), "path not found: " + str(query_path)
        pack = get_run_options(query_path.as_posix())
        return pack

    def run_packs(
        self, top_dir: Path, get_run_options: Callable[[str], RunWithTimeoutOptions]
    ):
        base_pack = QuerySpec.get_pack(top_dir, self.base, get_run_options)._replace(
            preprocessors=self.preprocess
        )
        materialize_pack = (
            QuerySpec.get_pack(top_dir, self.materialize, get_run_options)._replace(
                preprocessors=self.preprocess
            )
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
                    capture_output=extra.capture_output,
                    extras=_extra_context,
                    skip_validation=extra.skip_validation,
                    strict_run=extra.strict_run,
                )
                if extra.preprocess:
                    extra_pack = extra_pack._replace(
                        preprocessors=[*extra_pack.preprocessors, *extra.preprocess]
                    )
                extra_result = _run_with_timeout(extra_pack)
                extra_results[extra.label] = extra_result
            return extra_results

        original_get_options = get_run_options

        def _new_get_run_options(*args, **kwargs):
            options = original_get_options(*args, **kwargs)
            return options._replace(skip_validation=True)

        iter_count = 0
        start_perf_counter = time.perf_counter()
        while True:
            # for iter in range(base_pack.params.repeat + base_pack.params.throwaway):
            if iter_count > 0:
                base_pack = base_pack._replace(skip_validation=True)
                if materialize_pack is not None:
                    materialize_pack = materialize_pack._replace(skip_validation=True)
                get_run_options = _new_get_run_options
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
            base_context = dict()
            base_pack = base_pack._replace(extras=base_context)
            base_time = _run_with_timeout(base_pack)
            materialize_time = None
            if base_time is None:
                results["base"].append(dict(timeout=True))
                break

            base_extras_to_run = [
                extra for extra in self.extras if extra.runs_after_base
            ]

            base_extra_results = _run_extras(base_extras_to_run, base_context)
            base_pack.close_all()
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

            materialize_extra_results = {}

            if materialize_time:
                results["materialize"].append(materialize_time)
                mat_extras_to_run = [
                    extra for extra in self.extras if extra.runs_after_materialize
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

    def get_as_dict(self):
        return {**self._asdict(), "extras": [extra._asdict() for extra in self.extras]}


class ValidationQuerySpec(QuerySpec):

    def run_packs(self, top_dir, get_run_options):
        print("[validation]: ", self.base, self.materialize)
        base_pack = ValidationQuerySpec.get_pack(top_dir, self.base, get_run_options)
        other_pack = ValidationQuerySpec.get_pack(
            top_dir, self.materialize, get_run_options
        )

        base_result = f"psql {base_pack.connection_params.get_flat()} -A --field-separator='|' -P \"footer=off\" -f {base_pack.file_path} > /tmp/traceprov_base.out"
        other_result = f"psql {other_pack.connection_params.get_flat()} -A --field-separator='|' -P \"footer=off\" -f {other_pack.file_path} > /tmp/traceprov_other.out"
        assert os.system(base_result) == 0
        assert os.system(other_result) == 0
        diff_return = os.system(
            "diff -u /tmp/traceprov_base.out /tmp/traceprov_other.out"
        )
        assert diff_return == 0
        return diff_return


class Query(NamedTuple):
    query_name: str
    spec: QuerySpec

    def get_as_dict(self) -> dict[str, Any]:
        return {**self._asdict(), "spec": self.spec._asdict()}


class QueryDirectory(NamedTuple):
    dir_name: str
    queries: list[Query]

    def get_as_dict(self) -> dict[str, Any]:
        return {
            **self._asdict(),
            "queries": [query.get_as_dict() for query in self.queries],
        }


class GenericBenchmark(NamedTuple):
    name: str

    call_options: (
        None | Tuple[str, list[QueryDirectory], ConnectionParams, RunParams]
    ) = None

    traceprov_path: str = None
    traceprov_infer_set_path: str = None

    def run_from_argparse(
        self, directories: list[QueryDirectory], params=RunParams(), parser=None
    ):
        assert self.traceprov_path is None and self.traceprov_infer_set_path is None
        if parser is None:
            parser = argparse.ArgumentParser(prog=f"run-{self.name}")
        postgres_connection_from_cmd(parser)
        parser.add_argument("-suff", "--suff", required=True)
        parser.add_argument("-tp_root", "--traceprov_root", required=True)
        parser.add_argument("-t_root", "--test_root", required=True)
        parser.add_argument("-pg_version", required=int)
        parser.add_argument('--use_def', action=argparse.BooleanOptionalAction, default=True)

        parsed, _ = parser.parse_known_args()
        connection_params = ConnectionParams(
            host=parsed.host,
            port=parsed.port,
            user=parsed.user,
            password=parsed.password,
            database=parsed.db,
        )
        setup_bench = self.setup(
            parsed.traceprov_root, connection_params, parsed.suff, parsed.pg_version, parsed.use_def
        )

        start = time.perf_counter()
        called_benchmark, result = setup_bench.run(
            parsed.test_root,
            directories,
            connection_params,
            params,
        )
        end = time.perf_counter()
        final_result = dict(
            result=result,
            time_taken=end - start,
            db=parsed.db,
            stats=StatsCollector().collect_stats("postgres", connection_params),
            called_benchmark=called_benchmark,
            prefix=parsed.suff,
        )
        return final_result

    def dump_final_result(self, final_result: dict, out_dir="./results/"):
        os.makedirs(out_dir, exist_ok=True)
        current_timestamp = datetime.now()
        datetime_string = current_timestamp.strftime("%Y_%m_%d_%H_%M_%S")
        result_dir = f"{out_dir}/{final_result['prefix']}_{datetime_string}/"
        os.makedirs(result_dir, exist_ok=False)
        stats = final_result["stats"]
        called_benchmark: GenericBenchmark = final_result["called_benchmark"]
        other_result = {
            key: value
            for (key, value) in final_result.items()
            if key not in ["stats", "called_benchmark"]
        }
        with open(f"{result_dir}/stats.json", "w") as f:
            f.write(json.dumps(stats, indent=4, default=json_serial))

        with open(f"{result_dir}/main_result.json", "w") as f:
            f.write(json.dumps(other_result, indent=4, default=json_serial))

        with open(f"{result_dir}/benchmarks_params.json", "w") as f:
            f.write(
                json.dumps(
                    called_benchmark.get_as_dict(), indent=4, default=json_serial
                )
            )

    def setup(
        self,
        traceprov_postgres_root: str,
        connection_params: ConnectionParams,
        suff: str = None,
        pg_version: str = None,
        use_def: bool = True
    ):
        if pg_version is None:
            raise Exception("PG version is not set!")
        if suff is None:
            suff = self.name
        response = os.system(
            f"cd {traceprov_postgres_root} && make clean && make traceprov suff={suff} PG_VERSION={pg_version} USE_DEF={int(use_def)} && make infer_set suff={suff} PG_VERSION={pg_version} USE_DEF={int(use_def)}"
        )
        if response != 0:
            raise Exception("Make failed!")
        traceprov_sql = Path(traceprov_postgres_root) / f"traceprov_{suff}.auto.sql"
        traceprov_infer_set = (
            Path(traceprov_postgres_root) / f"traceprov_return_infer_{suff}.auto.sql"
        )
        assert traceprov_sql.exists(), f"{traceprov_sql.as_posix()} should exist!"
        assert (
            traceprov_infer_set.exists()
        ), f"{traceprov_infer_set.as_posix()} should exist!"

        assert (
            os.system(
                f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_sql.as_posix()} -v ON_ERROR_STOP=1"
            )
            == 0
        )

        assert (
            os.system(
                f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_infer_set.as_posix()} -v ON_ERROR_STOP=1"
            )
            == 0
        )

        traceprov_obj = f"traceprov_{suff}.dylib"
        traceprv_infer_set_obj = f"traceprov_return_{suff}.dylib"

        return self._replace(
            traceprov_path=traceprov_obj,
            traceprov_infer_set_path=traceprv_infer_set_obj,
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

        call_options = (top_dir, directories, connection_params, params)
        # params.validate()

        def _get_options(file_path: str):
            return RunWithTimeoutOptions(
                connection_params=connection_params,
                file_path=file_path,
                params=params,
            )

        results_from_dirs = {}
        directories_preprocess_applied = [
            self.setup_preprocess(directory) for directory in directories
        ]
        for directory in directories_preprocess_applied:
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

        return self._replace(call_options=call_options), results_from_dirs

    def get_as_dict(self):
        # Basically, recursively calls get as dict.
        assert self.call_options is not None

        base_serialized = dict(top_dir=self.call_options[0])
        directories_serialized = [
            directory.get_as_dict() for directory in self.call_options[1]
        ]
        connection_params_serialized = self.call_options[2]._asdict()
        runparams_serialized = self.call_options[3]._asdict()

        assert isinstance(self.call_options[2], ConnectionParams)
        assert isinstance(self.call_options[3], RunParams)

        serialized = {
            **base_serialized,
            **dict(
                directories=directories_serialized,
                connection_params=connection_params_serialized,
                runparams=runparams_serialized,
            ),
            **(self._asdict()),
        }

        return serialized

    def setup_preprocess(self, directory: QueryDirectory):
        def _map_preprocess(preprocess: Preprocessor):
            if not isinstance(preprocess, ReplaceFILE):
                raise Exception("Not implemented other preprocess yet")
            if preprocess.replace_with_token == "traceprov_path":
                return ReplaceFILE(self.traceprov_path)
            if preprocess.replace_with_token == "traceprov_infer_set_path":
                return ReplaceFILE(self.traceprov_infer_set_path)
            raise Exception(f"Unexpected token: {preprocess.replace_with_token}")

        def _setup_preprocess(query: Query):
            query_spec = query.spec
            new_extras = [
                extra._replace(
                    preprocess=[
                        _map_preprocess(preproc) for preproc in extra.preprocess
                    ]
                )
                for extra in query_spec.extras
            ]
            new_query_spec = query_spec._replace(extras=new_extras)
            return query._replace(spec=new_query_spec)

        new_queries = [_setup_preprocess(q) for q in directory.queries]
        return directory._replace(queries=new_queries)
