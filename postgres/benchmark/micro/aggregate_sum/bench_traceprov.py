import argparse

from traceprovpy.tools.benchmark import GenericBenchmark, Query, QueryDirectory, QuerySpec
from traceprovpy.tools.benchmark_utils import TRACEPROV_GET_LAYER_SIZE, TRACEPROV_INFER_SPEC
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import MakeTraceProv, RunParams
from traceprovpy.tools.traceprov_extra_func import TRACEPROV_DERIVE_OFFSET_KEY, TRACEPROV_LAYERS_TO_DERIVE_KEY, TRACEPROV_MATERIALIZE_LAYER_KEY, TRACEPROV_PROFILE_DUCKDB, TRACEPROV_USE_EXTRA_RESULT
from utils import SkewValue, get_replacers

def main():
    benchmark = GenericBenchmark("aggregate-sum-traceprov")
    parser = argparse.ArgumentParser("driver")
    parser.add_argument("--config", required=True, type=str)
    parser.add_argument("--keys_mode", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--mat_infer", action=argparse.BooleanOptionalAction, default=False)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    query_dirs = []
    extras = [TRACEPROV_INFER_SPEC(), TRACEPROV_GET_LAYER_SIZE()]
    for dir in config['dirs']:
        num_rows = dir['num_rows']
        assert isinstance(num_rows, str)
        skew_list = (dir['skew'])
        traceprov_queries = []
        for skew in skew_list:
            skew_value = SkewValue.deserialize(skew)
            skew_value_str = skew_value.serialize()
            preprocessor = get_replacers(num_rows, skew_value)
            query_name = f"skew_{skew_value_str}"
            base_query = Query(
                query_name=query_name,
                spec=QuerySpec(
                    base="$ROOT/queries/base.sql",
                    key="base",
                    preprocess=[*preprocessor],
                )
            )
            traceprov_query = Query(
                query_name=query_name,
                extra_commands=[],
                spec=QuerySpec(
                    base="$ROOT/queries/base.sql",
                    key="traceprov",
                    preprocess=[MakeTraceProv(), *preprocessor],
                    extras=extras,
                    extra_options={
                        TRACEPROV_LAYERS_TO_DERIVE_KEY: [1],
                        TRACEPROV_MATERIALIZE_LAYER_KEY: parsed.mat_infer,
                        TRACEPROV_DERIVE_OFFSET_KEY: not parsed.keys_mode,
                        TRACEPROV_PROFILE_DUCKDB: False,
                        TRACEPROV_USE_EXTRA_RESULT: False,
                    }
                )
            )
            traceprov_queries.append(base_query)
            traceprov_queries.append(traceprov_query)
        query_dirs.append(
            QueryDirectory(
                dir_name=num_rows,
                queries=traceprov_queries
            )
        )
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()