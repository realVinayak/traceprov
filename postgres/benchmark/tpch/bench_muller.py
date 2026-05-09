import argparse

from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    MULLER_GET_LOG_SIZE,
    TRACEPROV_MAKE_TRUNCATE_LOGS,
    traceprov_make_create_view,
    traceprov_make_drop_view,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, RunParams


def add_row(in_path: str, is_rows: bool):
    if not is_rows:
        return in_path
    if "." in in_path:
        return in_path.replace(".", "_row.")
    return in_path + "_row"


def make_query_15(query_name: str, is_rows: bool):
    # Need to handle query 15 in special way.
    key = "traceprov_15_skippable"
    phase_1_view = "revenue0_1"
    phase_2_view = add_row("revenue0_2", is_rows)
    query = Query(
        query_name=query_name,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key=key,
            extras=[
                traceprov_make_drop_view("drop_view_phase_1", phase_1_view, False),
                traceprov_make_drop_view("drop_view_phase_2", phase_2_view, False),
                traceprov_make_create_view(
                    "create_view_phase_1", add_row("phase_1_view.sql", is_rows)
                ),
                ExtraQuery(
                    label="phase_1_capture",
                    query="phase_1.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                ),
                traceprov_make_create_view(
                    "create_view_phase_2", add_row("phase_2_view.sql", is_rows)
                ),
                ExtraQuery(
                    label="phase_2_capture",
                    query=add_row("phase_2.sql", is_rows),
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                ),
                MULLER_GET_LOG_SIZE(),
                traceprov_make_drop_view("drop_view_phase_1", phase_1_view, True),
                traceprov_make_drop_view("drop_view_phase_2", phase_2_view, True),
                TRACEPROV_MAKE_TRUNCATE_LOGS(),
            ],
        ),
        extra_commands=["select truncateLogs();"],
    )
    return query


def make_query(query_name: str, is_rows: bool):
    if query_name == "15":
        return make_query_15(query_name, is_rows)
    return Query(
        query_name=query_name,
        spec=QuerySpec(
            base="phase_1.sql",
            key="phase_1_2_combined",
            materialize=add_row("phase_2.sql", is_rows),
            extras=[MULLER_GET_LOG_SIZE(), TRACEPROV_MAKE_TRUNCATE_LOGS()],
        ),
    )


def main():
    benchmark = GenericBenchmark("tpch-driver-muller")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    # whether to use row based rather than cell based.
    parser.add_argument("--row", action=argparse.BooleanOptionalAction, default=True)
    parsed, _ = parser.parse_known_args()

    config = json_read_file(parsed.config)
    assert config is not None

    query_repr = config["queries"]
    queries = []
    for query_name in map(str, query_repr):
        print(query_name)
        queries.append(make_query(query_name, parsed.row))

    subdir = config["subdir"]

    query_dir = QueryDirectory(dir_name=subdir, queries=queries)
    result = benchmark.run_from_argparse(
        [query_dir],
        RunParams(**config.get("runTimeOptions", {})),
        init_sql=["select truncateLogs();"],
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
