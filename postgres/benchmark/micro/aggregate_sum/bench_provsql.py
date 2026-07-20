import argparse

from utils import SkewValue, get_replacers
from traceprovpy.tools.benchmark import ExtraQuery, GenericBenchmark, Query, QueryDirectory, QuerySpec
from traceprovpy.tools.benchmark_utils import PROVSQL_EXTRA_COMMANDS
from traceprovpy.tools.callable_repr import CallableRepr
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, ReplaceSelectivity, RunParams, RunWithTimeoutOptions, run_with_timeout


def handle_provsql_extra(extra_context):
    is_global_last = extra_context["is_global_last"]
    if not is_global_last:
        return dict(early_result=True)
    original_extra_pack: RunWithTimeoutOptions = extra_context["extra_query_spec"]
    print(extra_context)
    capture_result = extra_context["extra_results"][
        "phase_1_capture"
    ][0]
    if capture_result is None:
        return dict(timeout=True)
    captured_rows = capture_result["captured"]
    output_row_results = []
    for output_row_id, output_row in enumerate(captured_rows):
        provsql = output_row['provsql']
        selection_preprocessor = ReplaceSelectivity(provsql, ":provsql", True)
        filtered_extra_pack = original_extra_pack._replace(
            preprocessors=[
                *(original_extra_pack.preprocessors or []),
                selection_preprocessor
            ],
            capture_output=False,
            strict_run=False,
        )
        tuid_results = []
        total_iter_count = (
            original_extra_pack.params.repeat + original_extra_pack.params.throwaway
        )
        for extra_iter in range(total_iter_count):
            print("EXTRA ITER: ", extra_iter)
            tuid_result = run_with_timeout(filtered_extra_pack)
            tuid_results.append(tuid_result)
        tuid_result_pack = dict(row_id=output_row_id, provsql=[provsql], results=tuid_results)
        output_row_results.append(tuid_result_pack)
    return dict(backtrace_results=output_row_results)

def make_query_single_row_mode(query_name: str, phase_1_query, phase_2_query, preprocess):
    query = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="provsql_backtrace_offset",
            extras=[
                ExtraQuery(
                    label="phase_1_capture",
                    query=phase_1_query,
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=True,
                    strict_run=False,
                    use_dict_cursor=True,
                    preprocess=(preprocess)
                ),
                ExtraQuery(
                    label="phase_2_capture",
                    query=phase_2_query,
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                    func=CallableRepr(handle_provsql_extra, "handle_provsql_extra"),
                    preprocess=preprocess
                ),
            ]
        ),
        extra_commands=[*PROVSQL_EXTRA_COMMANDS]
    )
    return query


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
    dirs = config['dirs']
    query_dirs = []
    for dir in dirs:
        num_rows = dir['num_rows']
        assert isinstance(num_rows, str)
        skew_list = (dir['skew'])
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
                        base="$ROOT/queries/base.sql",
                        key="base",
                        preprocess=[*preprocessor],
                    ),
                    extra_commands=[*PROVSQL_EXTRA_COMMANDS]
                )
                provsql_backtrace_all = Query(
                    query_name=query_name,
                    spec=QuerySpec(
                        base="$ROOT/queries/provsql_backtrace_all.sql",
                        key="provsql_backtrace_all",
                        preprocess=[*preprocessor],
                    ),
                    extra_commands=[*PROVSQL_EXTRA_COMMANDS]
                )
                provsql_queries.extend([provsql_capture_query, provsql_backtrace_all])
            else:
                provsql_backtrace_offset = make_query_single_row_mode(
                    query_name,
                    "$ROOT/queries/base.sql",
                    "$ROOT/queries/provsql_backtrace_offset.sql",
                    preprocessor
                )
                provsql_queries.append(provsql_backtrace_offset)
        query_dirs.append(
            QueryDirectory(
                dir_name=num_rows,
                queries=provsql_queries
            )
        )
    benchmark = benchmark._replace(skip_load=True)
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    # print(result)
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()