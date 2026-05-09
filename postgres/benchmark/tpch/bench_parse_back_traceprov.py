import os
from pathlib import Path
from typing import List
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    ValidationQuerySpec,
    bench_has_duckdb_infer,
)
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_CAPTURE_QUERY,
    TRACEPROV_GET_DERIVATION_SPEC,
    TRACEPROV_GET_GENERIC_DERIVATION_SPEC,
    TRACEPROV_GET_LAYER_SIZE,
    TRACEPROV_INFER_SPEC,
    TRACEPROV_PERFORM_DERIVATION,
    TRACEPROV_SYNC_TIME,
    traceprov_make_create_view,
    traceprov_make_drop_view,
)
from traceprovpy.tools.duckdb_inference import DuckDbInferenceBinQuerySpec
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    TP_SKIPPABLE_OPTION,
    MakeTraceProv,
    RunParams,
)
import json
import argparse

from traceprovpy.tools.traceprov_extra_func import (
    TRACEPROV_LAYERS_TO_DERIVE_KEY,
    TRACEPROV_MATERIALIZE_LAYER_KEY,
)


def special_query(
    query_name: str,
    is_traceprov: bool,
    extra_commands: list[str] = [],
    layers_to_derive=[],
    is_validate: bool = False,
):
    if query_name != "15":
        return None
    key = "traceprov_15_skippable" if is_traceprov else "base_15_skippable"
    assert not is_traceprov or len(layers_to_derive) > 0
    extra_commands = extra_commands or []
    extra_commands = [*extra_commands, *enable_join_choices()]
    return Query(
        query_name=query_name,
        extra_commands=extra_commands,
        spec=QuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key=key,
            extras=[
                traceprov_make_drop_view("15_pre_step_1", "revenue0", strict=False),
                traceprov_make_create_view("15_pre_step_2", "create_view.sql"),
                ExtraQuery(
                    label="traceprov" if is_traceprov else "base",
                    query="base.sql",
                    runs_after_base=True,
                    skip_validation=False,
                    capture_output=False,
                    strict_run=False,
                    preprocess=([MakeTraceProv()] if is_traceprov else []),
                ),
                *(
                    [TRACEPROV_INFER_SPEC(), TRACEPROV_GET_LAYER_SIZE()]
                    if is_traceprov
                    else []
                ),
                traceprov_make_drop_view("15_post_step_1", "revenue0", strict=True),
            ],
            extra_options=(
                {
                    TRACEPROV_LAYERS_TO_DERIVE_KEY: layers_to_derive,
                    TRACEPROV_MATERIALIZE_LAYER_KEY: is_validate,
                }
                if is_traceprov
                else None
            ),
        ),
    )


def set_hashjoin(switch: bool):
    value = "on" if switch else "off"
    return f"SET enable_hashjoin = {value};"


def set_mergejoin(switch: bool):
    value = "on" if switch else "off"
    return f"SET enable_mergejoin = {value};"


def disable_join_choices():
    return [set_hashjoin(False), set_mergejoin(False)]


def enable_join_choices():
    return [set_hashjoin(True), set_mergejoin(True)]


def make_normal_query(
    query_name: str,
    is_traceprov=False,
    extra_commands: list[str] = None,
    layers_to_derive=[],
    is_validate=False,
    toggle_join_choices=True,
):
    if not is_traceprov:
        return Query(
            query_name=query_name,
            spec=QuerySpec(base="base.sql", key="base"),
            extra_commands=[*enable_join_choices()],
        )

    # don't need to check if we'll dump or not.
    # extras = [TRACEPROV_SYNC_TIME(), TRACEPROV_GET_DERIVATION_SPEC()]
    assert len(layers_to_derive) > 0
    extras = [TRACEPROV_INFER_SPEC(), TRACEPROV_GET_LAYER_SIZE()]
    extra_commands = extra_commands or []

    if int(query_name) == 4 and toggle_join_choices:
        extra_commands = [*extra_commands, *disable_join_choices()]
    else:
        extra_commands = [*extra_commands, *enable_join_choices()]
    return Query(
        query_name=query_name,
        extra_commands=extra_commands,
        spec=QuerySpec(
            base="base.sql",
            key="traceprov",
            preprocess=[MakeTraceProv()],
            extras=extras,
            extra_options={
                TRACEPROV_LAYERS_TO_DERIVE_KEY: layers_to_derive,
                TRACEPROV_MATERIALIZE_LAYER_KEY: is_validate,
            },
        ),
    )


def get_query(
    query_name: str,
    parsed,
    extra_commands: list[str] = [],
    layers_to_derive=[],
):
    is_validate = parsed.validate
    is_dump_graph_mode = parsed.dump_graph_mode
    subdir_queries: List[Query] = []

    special_query_maybe = special_query(
        query_name, is_traceprov=False, extra_commands=extra_commands
    )
    if special_query_maybe:
        subdir_queries.append(special_query_maybe)
        special_query_traceprov = special_query(
            query_name,
            is_traceprov=True,
            extra_commands=extra_commands,
            layers_to_derive=layers_to_derive,
            is_validate=is_validate,
        )
        assert special_query_traceprov is not None
        subdir_queries.append(special_query_traceprov)
    else:
        subdir_queries.append(
            make_normal_query(
                query_name, is_traceprov=False, extra_commands=extra_commands
            )
        )
        subdir_queries.append(
            make_normal_query(
                query_name,
                is_traceprov=True,
                extra_commands=extra_commands,
                layers_to_derive=layers_to_derive,
                is_validate=is_validate,
            )
        )

    if is_dump_graph_mode:
        opt_choice = "optimized" if parsed.use_optimized_query else "non_optimized"
        out_dir = Path(parsed.dump_graph_dir) / opt_choice / query_name
        os.makedirs(out_dir, exist_ok=True)
        subdir_queries.append(
            Query(
                query_name=query_name,
                spec=DuckDbInferenceBinQuerySpec(
                    base=TP_SKIPPABLE_OPTION,
                    key="bin_copy",
                    extra_options=dict(destination_dir=out_dir),
                ),
            )
        )

    if is_validate:
        subdir_queries.append(
            Query(
                query_name=query_name,
                spec=ValidationQuerySpec(
                    base="base.sql",
                    key="VALIDATION",
                    materialize="validate_rel_infer.sql",
                ),
            )
        )

    return subdir_queries


# This parses out the config file, and generates the directories
# on the fly, to be used.
# We log the directories later, anyways, so this is fine.
def main():
    benchmark = GenericBenchmark("tpch-driver")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument(
        "--use_optimized_query", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--derive_config", required=True, type=str)
    parser.add_argument(
        "--validate", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--dump_graph_mode", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--toggle_join_choices", action=argparse.BooleanOptionalAction, default=True
    )
    parser.add_argument("--dump_graph_dir", default="./tmp/", required=False)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    derive_config = json_read_file(parsed.derive_config)
    dir_queries = []

    extra_commands = []
    if parsed.use_optimized_query:
        extra_commands = ["set traceprov.use_rowid_duckdb=on;"]
    for subdir in config["subdirs"]:
        subdir_queries = []
        queries = config["queries"]
        if isinstance(queries, str):
            queries = eval(queries)
        for query_name in queries:
            query_name = str(query_name)
            subdir_queries = [
                *subdir_queries,
                *get_query(
                    query_name,
                    parsed,
                    extra_commands,
                    layers_to_derive=derive_config[query_name]["layers_used"],
                ),
            ]
        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
