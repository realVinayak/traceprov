import argparse

from traceprovpy.tools.callable_repr import CallableRepr
from utils import SkewValue, get_replacers
from traceprovpy.tools.benchmark import GenericBenchmark, QueryDirectory, QueryLimitQuerySpec, QuerySpec
from traceprovpy.tools.benchmark_utils import make_gprom_query
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import ReplaceSelectivity, RunParams

def _add_gprom(curr: str):
    assert '.gprom.sql' not in curr
    return curr.replace('.sql', '.gprom.sql')

def base_getter(query):
    return f"$ROOT/queries/base.sql"

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
    query_class = QuerySpec
    query_dir = "queries/gprom_all"
    if parsed.keys_mode:
        query_class = QueryLimitQuerySpec
        query_dir = "queries/gprom_keys"
    query_dirs = []
    for dir in dirs:
        num_rows = (dir['num_rows'])
        assert isinstance(num_rows, str)
        skew_list = (dir['skew'])
        gprom_queries = []
        for skew in skew_list:
            print("SKEW: ", skew)
            skew_value = SkewValue.deserialize(skew)
            skew_value_str = skew_value.serialize()
            gprom_query = make_gprom_query(f"skew_{skew_value_str}", "all", True, query_class, ['z'])
            preprocessor = get_replacers(num_rows, skew_value)
            gprom_adjusted = [query._replace(spec=query.spec._replace(base=_add_gprom(f"$ROOT/{query_dir}/{query.spec.base}"), preprocess=preprocessor, extra_options=({**query.spec.extra_options, "base_getter": CallableRepr(base_getter, "base_getter_gprom")}))) for query in gprom_query]
            gprom_queries.extend(gprom_adjusted)
        query_dirs.append(
            QueryDirectory(
                dir_name=num_rows,
                queries=gprom_queries
            )
        )
    
    result = benchmark.run_from_argparse(
        query_dirs, params=RunParams(**config.get("runTimeOptions", {}))
    )
    # print(result)
    benchmark.dump_final_result(result)

if __name__ == '__main__':
    main()