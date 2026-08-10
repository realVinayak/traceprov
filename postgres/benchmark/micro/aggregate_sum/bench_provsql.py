import argparse

from utils import SkewValue, get_replacers
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    PROVSQL_BACKTRACE_OFFSET,
    PROVSQL_BACKUP_WRITER,
    PROVSQL_EXTRA_COMMANDS,
    PROVSQL_LOG_SIZE_SPEC,
    PROVSQL_RESTART_WRITER,
)
from traceprovpy.tools.callable_repr import CallableRepr
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    TP_SKIPPABLE_OPTION,
    ReplaceSelectivity,
    RunParams,
    RunWithTimeoutOptions,
    run_with_timeout,
)


def make_query_single_row_mode(
    query_name: str, phase_1_query, phase_2_query, preprocess
):
    query_phase_1 = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="phase_1",
            extras=[
                PROVSQL_BACKUP_WRITER(),
                ExtraQuery(
                    label="capture",
                    query=phase_1_query,
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                    preprocess=(preprocess),
                ),
                PROVSQL_LOG_SIZE_SPEC(),
                PROVSQL_RESTART_WRITER(),
            ],
        ),
        extra_commands=[*PROVSQL_EXTRA_COMMANDS],
    )
    provsql_backtrace_extra_query = PROVSQL_BACKTRACE_OFFSET(None, None)
    provsql_backtrace_extra_query = provsql_backtrace_extra_query._replace(
        query=phase_2_query,
        preprocess=(preprocess),
    )
    query_phase_2 = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="phase_2",
            extras=[
                PROVSQL_BACKUP_WRITER(),
                ExtraQuery(
                    label="backtrace_capture",
                    query=phase_1_query,
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=True,
                    strict_run=False,
                    use_dict_cursor=True,
                    preprocess=(preprocess),
                ),
                provsql_backtrace_extra_query,
                PROVSQL_LOG_SIZE_SPEC(),
                PROVSQL_RESTART_WRITER(),
            ],
        ),
        extra_commands=[*PROVSQL_EXTRA_COMMANDS],
    )

    return [query_phase_1, query_phase_2]


def main():
    benchmark = GenericBenchmark("top-k-gprom")
    parser = argparse.ArgumentParser("driver")
    parser.add_argument("--config", required=True, type=str)
    parser.add_argument(
        "--keys_mode", action=argparse.BooleanOptionalAction, default=False
    )
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None
    dirs = config["dirs"]
    query_dirs = []
    for dir in dirs:
        num_rows = dir["num_rows"]
        assert isinstance(num_rows, str)
        skew_list = dir["skew"]
        provsql_queries = []
        for skew in skew_list:
            skew_value = SkewValue.deserialize(skew)
            skew_value_str = skew_value.serialize()
            preprocessor = get_replacers(num_rows, skew_value)
            query_name = f"skew_{skew_value_str}"
            if not parsed.keys_mode:
                provsql_capture_query = Query(
                    query_name=query_name,
                    spec=QuerySpec(
                        base=TP_SKIPPABLE_OPTION,
                        key="phase_1",
                        extras=[
                            PROVSQL_BACKUP_WRITER(),
                            ExtraQuery(
                                label="capture",
                                query="$ROOT/queries/base.sql",
                                runs_after_base=True,
                                skip_validation=False,
                                capture_output=False,
                                strict_run=False,
                                preprocess=[*preprocessor],
                            ),
                            PROVSQL_LOG_SIZE_SPEC(),
                            PROVSQL_RESTART_WRITER(),
                        ],
                        preprocess=[*preprocessor],
                    ),
                    extra_commands=[*PROVSQL_EXTRA_COMMANDS],
                )
                provsql_backtrace_all = Query(
                    query_name=query_name,
                    spec=QuerySpec(
                        base=TP_SKIPPABLE_OPTION,
                        key="phase_1_2",
                        extras=[
                            PROVSQL_BACKUP_WRITER(),
                            ExtraQuery(
                                label="capture_and_backtrace",
                                query="$ROOT/queries/provsql_backtrace_all.sql",
                                runs_after_base=True,
                                skip_validation=False,
                                capture_output=False,
                                strict_run=False,
                                preprocess=[*preprocessor],
                            ),
                            PROVSQL_LOG_SIZE_SPEC(),
                            PROVSQL_RESTART_WRITER(),
                        ],
                        preprocess=[*preprocessor],
                    ),
                    extra_commands=[*PROVSQL_EXTRA_COMMANDS],
                )
                provsql_queries.extend([provsql_capture_query, provsql_backtrace_all])
            else:
                provsql_backtrace_offset = make_query_single_row_mode(
                    query_name,
                    "$ROOT/queries/base.sql",
                    "$ROOT/queries/provsql_backtrace_offset.sql",
                    preprocessor,
                )
                provsql_queries.extend(provsql_backtrace_offset)
        query_dirs.append(QueryDirectory(dir_name=num_rows, queries=provsql_queries))
    benchmark = benchmark._replace(skip_load=True)
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    # print(result)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
