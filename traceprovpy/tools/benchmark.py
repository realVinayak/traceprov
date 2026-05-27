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

from collections import defaultdict
from typing import Any, Callable, NamedTuple, Tuple
from traceprovpy.tools.callable_repr import CallableRepr
from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.file_utils import just_write, traceprov_assert_safe_run
from traceprovpy.tools.run_with_timeout import (
    TP_SKIPPABLE_OPTION,
    ConnectionParams,
    MakeKeySelection,
    MakeLimitOne,
    MakeTraceProv,
    MatMaterialize,
    Preprocessor,
    ReplaceBucket,
    ReplaceFILE,
    ReplaceSelectivity,
    RunParams,
    SkippableRunTimeOptions,
    SmokedDuckOptions,
    run_with_timeout,
    RunWithTimeoutOptions,
)
from pathlib import Path, PosixPath
import os
import argparse
import time
import json
from datetime import date, datetime

from traceprovpy.tools.setup import traceprov_reinit_state, traceprov_setup
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

    if isinstance(obj, ReplaceBucket):
        return dict(preprocessor_type=ReplaceBucket.__name__, param="<truncated>")

    if isinstance(obj, MatMaterialize):
        return dict(preprocessor_type=MatMaterialize.__name__, param=obj.table_name)

    if isinstance(obj, MakeTraceProv):
        return "MakeTraceProv"

    if isinstance(obj, PosixPath):
        return obj.as_posix()

    if isinstance(obj, decimal.Decimal):
        return str(obj)

    if isinstance(obj, CallableRepr):
        return f"CallableRepr({obj.name})"

    raise TypeError("Type %s not serializable" % type(obj))


def _run_with_timeout(options: RunWithTimeoutOptions):
    print(options.file_path)
    if isinstance(options, SkippableRunTimeOptions):
        return dict(skipped=True)
    return run_with_timeout(options)


class ExtraQuery(NamedTuple):
    label: str
    query: str
    # We allow some arbitrary queries to run, right after a previous query.
    runs_after_base: bool = False
    runs_after_materialize: bool = False
    skip_validation: bool = False
    preprocess: list[Preprocessor] | None = None
    capture_output: bool = True
    strict_run: bool = False
    # Sometimes, it's benefecial to run extra multiple times too.
    repeat: int = 1
    # Allows running arbitrary functions in an ExtraQuery.
    # Useful for the dynamic infer (where we don't need to memoize the "spec")
    func: CallableRepr | None = None


OPTION_GETTER = Callable[[str], RunWithTimeoutOptions]


class QuerySpec(NamedTuple):
    key: str
    base: str
    materialize: str | None = None
    extras: list[ExtraQuery] = []
    preprocess: list[Preprocessor] = []
    # random options.
    extra_options: dict = None

    @staticmethod
    def get_pack(
        top_dir: Path,
        path: str,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
    ):
        print(path)
        if path == TP_SKIPPABLE_OPTION:
            return SkippableRunTimeOptions(*get_run_options(path))
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
        self,
        top_dir: Path,
        get_run_options: Callable[[str], RunWithTimeoutOptions],
        benchmark: "GenericBenchmark",
    ) -> Any:
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
            extra_results = defaultdict(list)
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
                        preprocessors=[
                            *(extra_pack.preprocessors or []),
                            *(extra.preprocess or []),
                        ]
                    )

                # Assume that func is smart enough to handle the repeat correctly.
                for _ in range(extra.repeat if not extra.func else 1):
                    if extra.func:
                        extra_result = extra.func(
                            self, extra, extra_pack, get_run_options
                        )
                    else:
                        extra_result = _run_with_timeout(extra_pack)
                    extra_results[extra.label].append(extra_result)
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
            traceprov_reinit_state(base_pack.connection_params)
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
            is_timeout = False
            if materialize_pack:
                materialize_pack = materialize_pack._replace(extras=materialize_context)
                materialize_time = _run_with_timeout(materialize_pack)
                assert len(materialize_context) != 0
                is_timeout = materialize_time is None

            if (
                base_pack.params.throwaway is not None
                and iter_count < base_pack.params.throwaway
            ):
                if materialize_pack:
                    materialize_pack.close_all()
                iter_count += 1
                # Break just at throwaway here.
                if is_timeout:
                    results["materialize"].append(dict(timeout=True))
                    break
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
            # if the materialize query also timed out, also break out
            if is_timeout:
                results["materialize"].append(dict(timeout=True))
                break

        return results

    def get_as_dict(self):
        return {**self._asdict(), "extras": [extra._asdict() for extra in self.extras]}


class ValidationQuerySpec(QuerySpec):

    def run_packs(self, top_dir, get_run_options, benchmark):
        print("[validation]: ", self.base, self.materialize)
        base_pack = self.get_pack(top_dir, self.base, get_run_options)
        assert self.materialize
        other_pack = self.get_pack(top_dir, self.materialize, get_run_options)

        base_result = f"psql {base_pack.connection_params.get_flat()} -A --field-separator='|' -P \"footer=off\" -f {base_pack.file_path} > /tmp/traceprov_base.out"
        other_result = f"psql {other_pack.connection_params.get_flat()} -A --field-separator='|' -P \"footer=off\" -f {other_pack.file_path} > /tmp/traceprov_other.out"
        traceprov_assert_safe_run(base_result)
        traceprov_assert_safe_run(other_result)
        traceprov_assert_safe_run(
            "diff -u /tmp/traceprov_base.out /tmp/traceprov_other.out"
        )
        return 0


class DropTable(QuerySpec):

    def run_packs(self, top_dir, get_run_options, benchmark):
        tables = self.base.split(":")
        base_pack = self.get_pack(top_dir, TP_SKIPPABLE_OPTION, get_run_options)
        print("[drpp table]: ", self.base)
        con = base_pack.connection_params.make_connection()
        cursor = con.cursor()
        for cmd in base_pack.extra_commands or []:
            cursor.execute(cmd)
        for table in tables:
            cursor.execute(f"drop table if exists {table}")
        cursor.close()
        con.commit()
        con.close()
        return 0


class SketchValidationQuerySpec(QuerySpec):

    def run_packs(self, top_dir, get_run_options, benchmark):
        base_pack = self.get_pack(top_dir, TP_SKIPPABLE_OPTION, get_run_options)
        print("[sketch validation]: ", self.base, self.materialize)
        query = f"select 1 from (select left_record FROM (select * from {self.base} limit 1) as left_record) JOIN (SELECT right_record FROM (select * from {self.materialize}) as right_record) ON left_record = right_record;"
        con = base_pack.connection_params.make_connection()
        cursor = con.cursor()
        cursor.execute(query)
        result = cursor.fetchone()
        assert result is not None and result == ((1,))
        cursor.close()
        con.close()
        return 0


class QueryLimitQuerySpec(QuerySpec):

    def run_packs(self, top_dir, get_run_options, benchmark):
        original_pack = self.get_pack(top_dir, self.base, get_run_options)
        back_pack = original_pack._replace(
            use_dict_cursor=True,
            capture_output=True,
            strict_run=True,
            preprocessors=[
                *(original_pack.preprocessors or []),
                MakeLimitOne(offset=0),
            ],
        )
        result = _run_with_timeout(back_pack)
        if result is None:
            return dict(timeout=True)
        filtered_dict = {
            key: value
            for (key, value) in result["captured"][0].items()
            if not (key.lower().startswith("prov_"))
        }
        make_selection_preprocessor = MakeKeySelection(filter_pack=filtered_dict)
        new_query = QuerySpec(*self)
        new_query = new_query._replace(
            preprocess=[*(self.preprocess or []), make_selection_preprocessor]
        )
        return new_query.run_packs(top_dir, get_run_options, benchmark)


class Query(NamedTuple):
    query_name: str
    spec: QuerySpec
    extra_commands: list[str] | None = []

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


def bench_has_bool_option(option: str):
    def _checker(args: list[str]):
        if f"--{option}" in args:
            return True
        if f"--no-{option}" in args:
            return False
        return False

    return _checker


INFER_DUCKDB_OPTION = "infer_duckdb"
SMOKEDDUCK_OPTION = "use_sdb"

bench_has_duckdb_infer = bench_has_bool_option(INFER_DUCKDB_OPTION)
bench_has_smokedduck = bench_has_bool_option(SMOKEDDUCK_OPTION)


class TraceProvOptimizations(NamedTuple):
    traceprov_use_table_stats: bool = False

    @staticmethod
    def _bool_to_switch(val: bool):
        return "on" if val else "off"

    def get_options(self):
        options = []
        if self.traceprov_use_table_stats:
            options.append(("traceprov.use_table_stats", True))

        return [f"set {opt[0]}={self._bool_to_switch(opt[1])}" for opt in options]

    @classmethod
    def make_from_parsed(cls, parsed):
        kwargs = dict()
        for field in TraceProvOptimizationsInstance._fields:
            kwargs[field] = getattr(parsed, field)
        return cls(**kwargs)


TraceProvOptimizationsInstance = TraceProvOptimizations()


# Warning: AI generated.
def delete_traceprov_tables(conn, shared_libraries: list[str]):
    with conn.cursor() as cur:
        for shared_lib in shared_libraries:
            print("Loading: ", shared_lib)
            cur.execute(f"load '{shared_lib}';")
        cur.execute("""
            SELECT tablename 
            FROM pg_tables 
            WHERE schemaname = 'public' 
              AND tablename LIKE 'traceprov_relation_infer_%'
        """)
        tables = cur.fetchall()

        if not tables:
            print("No matching tables found.")
            return

        for (table_name,) in tables:
            cur.execute(f'DROP TABLE IF EXISTS "{table_name}" CASCADE')
            print(f"Dropped table: {table_name}")

        conn.commit()
        print(f"Done. {len(tables)} table(s) dropped.")


class GenericBenchmark(NamedTuple):
    name: str

    call_options: (
        None | Tuple[str, list[QueryDirectory], ConnectionParams, RunParams]
    ) = None

    traceprov_path: None | str = None
    traceprov_infer_set_path: None | str = None
    traceprov_rewriter_path: None | str = None
    sd_options: SmokedDuckOptions | None = None
    skip_load: bool = False

    def run_from_argparse(
        self,
        directories: list[QueryDirectory],
        params=RunParams(),
        parser=None,
        init_sql: list[str] = None,
        can_skip_build=False,
    ):
        if len(directories) == 0:
            raise Exception("Trying to run test without any dirs!")
        assert (
            self.traceprov_path is None
            and self.traceprov_infer_set_path is None
            and self.traceprov_rewriter_path is None
        )
        if parser is None:
            parser = argparse.ArgumentParser(prog=f"run-{self.name}")
        postgres_connection_from_cmd(parser)
        parser.add_argument("-suff", "--suff", required=True)
        parser.add_argument("-tp_root", "--traceprov_root", required=True)
        parser.add_argument("-t_root", "--test_root", required=True)
        parser.add_argument("-sd_lib", required=False, type=str)
        parser.add_argument("-sd_include", required=False, type=str)
        parser.add_argument("-sd_num_threads", required=False, type=int)
        parser.add_argument(
            f"--{INFER_DUCKDB_OPTION}",
            action=argparse.BooleanOptionalAction,
            default=False,
        )
        parser.add_argument(
            f"--{SMOKEDDUCK_OPTION}",
            action=argparse.BooleanOptionalAction,
            default=False,
        )
        parser.add_argument(
            f"--sd_create_idx",
            action=argparse.BooleanOptionalAction,
            default=False,
        )

        for optimizations in TraceProvOptimizationsInstance._fields:
            parser.add_argument(
                f"--{optimizations}",
                action=argparse.BooleanOptionalAction,
                default=TraceProvOptimizationsInstance._field_defaults[optimizations],
            )
        parsed, _ = parser.parse_known_args()

        connection_params = ConnectionParams(
            host=parsed.host,
            port=parsed.port,
            user=parsed.user,
            password=parsed.password,
            database=parsed.db,
        )
        setup_bench = self.setup(
            parsed.traceprov_root,
            connection_params,
            parsed.suff,
            parsed.sd_lib,
            parsed.sd_include,
            parsed.sd_num_threads,
            parsed.sd_create_idx,
            can_skip_build,
        )
        local_optimization_instance = TraceProvOptimizations.make_from_parsed(parsed)
        start = time.perf_counter()
        called_benchmark, result = setup_bench.run(
            parsed.test_root,
            directories,
            connection_params,
            params,
            init_sql,
            self_extra_sql=local_optimization_instance.get_options(),
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

        if "extras" in final_result:
            raise Exception('Expected "extras" to be a reserved keyword.')
        connection = connection_params.make_connection()
        delete_traceprov_tables(
            connection,
            [
                setup_bench.traceprov_rewriter_path,
                setup_bench.traceprov_path,
                setup_bench.traceprov_infer_set_path,
            ],
        )
        connection.close()
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
        suff: str | None = None,
        # the smokedduck shared library.
        sd_lib_path: str = "",
        sd_include_path: str = "",
        sd_num_threads: int | None = None,
        sd_create_idx: bool = False,
        can_skip_build=False,
    ):
        setup_response = traceprov_setup(
            suff or self.name,
            traceprov_postgres_root,
            connection_params,
            sd_lib_path,
            sd_include_path,
            sd_num_threads,
            sd_create_idx,
            can_skip_build,
        )

        return self._replace(**setup_response)

    def run(
        self,
        top_dir: str,
        directories: list[QueryDirectory],
        connection_params: ConnectionParams,
        params=RunParams(),
        init_sql: list[str] = None,
        self_extra_sql: list[str] = None,
    ):
        print("Extra SQL: ", self_extra_sql)
        # Always run the analyze for statistics initially.
        assert (
            os.system(
                f'echo "ANALYZE;" | PGPASSWORD={connection_params.password} psql {connection_params.get_flat()}'
            )
            == 0
        )
        if init_sql:
            init_sql_joined = ";\n".join(init_sql)
            init_sql_path = just_write("/tmp/init_sql.sql", init_sql_joined)
            assert (
                os.system(
                    f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {init_sql_path}"
                )
                == 0
            )
        print(directories)

        call_options = (
            top_dir,
            directories,
            connection_params,
            params,
            init_sql,
            self_extra_sql,
        )
        # params.validate()

        def _get_options_from_query(query: Query):
            def _get_options_from_file(file_path: str):
                assert self.traceprov_rewriter_path
                return RunWithTimeoutOptions(
                    connection_params=connection_params,
                    file_path=file_path,
                    params=params,
                    shared_libraries=(
                        [
                            self.traceprov_rewriter_path,
                        ]
                        if not self.skip_load
                        else []
                    ),
                    extra_commands=[
                        *(self_extra_sql or []),
                        *(query.extra_commands or []),
                    ],
                )

            return _get_options_from_file

        results_from_dirs = {}
        directories_preprocess_applied = [
            self.setup_preprocess(directory) for directory in directories
        ]
        for directory in directories_preprocess_applied:
            combined_results = {}
            for query in directory.queries:
                print(f"[{self.name}: ({directory.dir_name}, {query.query_name})]")
                _get_options = _get_options_from_query(query)
                results = query.spec.run_packs(
                    Path(top_dir) / directory.dir_name / query.query_name,
                    _get_options,
                    self,
                )
                current_query_result = combined_results.get(query.query_name, {})
                assert (
                    query.spec.key not in current_query_result
                ), f"Didn't expect {query.spec.key} to be in the result!"
                combined_results[query.query_name] = {
                    **current_query_result,
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
            skipable = [
                MakeTraceProv,
                ReplaceBucket,
                MatMaterialize,
                ReplaceSelectivity,
            ]
            if preprocess.__class__ in skipable:
                return preprocess
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
                        _map_preprocess(preproc) for preproc in extra.preprocess or []
                    ]
                )
                for extra in query_spec.extras
            ]
            new_query_spec = query_spec._replace(extras=new_extras)
            return query._replace(spec=new_query_spec)

        new_queries = [_setup_preprocess(q) for q in directory.queries]
        return directory._replace(queries=new_queries)
