import argparse
from typing import Literal

from utils import FRAME_SIZES
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import TRACEPROV_GET_LAYER_SIZE, TRACEPROV_INFER_SPEC
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    MakeTraceProv,
    ReplaceSelectivity,
    RunParams,
)
from traceprovpy.tools.traceprov_extra_func import (
    TRACEPROV_DERIVE_OFFSET_KEY,
    TRACEPROV_LAYERS_TO_DERIVE_KEY,
    TRACEPROV_MATERIALIZE_LAYER_KEY,
    TRACEPROV_PROFILE_DUCKDB,
    TRACEPROV_USE_EXTRA_RESULT,
)


def get_replacers(num_rows):
    replacers = [ReplaceSelectivity(str(num_rows), "ROW_COUNT", is_strict=True)]
    return replacers

def get_group_replacer(group_num):
    replacers = [ReplaceSelectivity(str(group_num), "GROUP_NUM", is_strict=False)]
    return replacers


# you know what, this should be used everywhere where the queries are simple
def make_traceprov_from_base(base_query: Query, parsed):
    extras = [TRACEPROV_INFER_SPEC(), TRACEPROV_GET_LAYER_SIZE()]
    traceprov_query = base_query._replace(
        spec=base_query.spec._replace(
            key="traceprov",
            preprocess=[MakeTraceProv(), *base_query.spec.preprocess],
            extras=extras,
            extra_options={
                TRACEPROV_LAYERS_TO_DERIVE_KEY: [1],
                TRACEPROV_DERIVE_OFFSET_KEY: not parsed.keys_mode,
                TRACEPROV_PROFILE_DUCKDB: False,
                TRACEPROV_USE_EXTRA_RESULT: False,
            }
        )
    )
    return traceprov_query


def main():
    benchmark = GenericBenchmark('window-traceprov')
    parser = argparse.ArgumentParser("driver")
    parser.add_argument(
        "--keys_mode", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--config", required=True, type=str)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None
    dirs = config['dirs']
    query_dir = []
    for dir in dirs:
        num_row = int(dir)
        base_replacers = get_replacers(num_row)
        queries = []
        for frame_size in FRAME_SIZES:
            added_replacers = [*base_replacers, *get_group_replacer(frame_size)]
            query_name = f"fixed_frame_{frame_size}"
            base_query_path = "$ROOT/queries/constant_frame/base.sql"
            base_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base=base_query_path,
                    key="base",
                    preprocess=[*added_replacers]
                )
            )
            queries.append(base_query)
            queries.append(make_traceprov_from_base(base_query, parsed))
        def _get_varying(varying_mode: Literal["group"] | Literal['row']):
            base_query_path = f"$ROOT/queries/varying_frame_{varying_mode}/base.sql"
            query_name = f"varying_{varying_mode}"
            base_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base=base_query_path,
                    key="base",
                    preprocess=[*base_replacers]
                )
            )
            return [base_query, make_traceprov_from_base(base_query, parsed)]
        queries.extend(_get_varying("row"))
        queries.extend(_get_varying("group"))
        query_dir.append(
            QueryDirectory(
                dir_name=dir,
                queries=queries
            )
        )

    result = benchmark.run_from_argparse(
        query_dir, params=RunParams(**config.get("runTimeOptions", {}))
    )
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
