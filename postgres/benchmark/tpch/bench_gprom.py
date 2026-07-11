import argparse
from pathlib import Path

from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QueryLimitQuerySpec,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    add_gprom_candidates,
    infer_gprom_candidates,
    make_gprom_query,
    parse_queries,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import RunParams


def make_base_query(query_name: str):
    return Query(query_name=query_name, spec=QuerySpec(base="base.sql", key="base"))

def main():
    benchmark = GenericBenchmark("tpch-driver-gprom")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-t_root", "--test_root", required=True)
    parser.add_argument("--dir", required=True)
    parser.add_argument(
        "--keys_mode", action=argparse.BooleanOptionalAction, default=False
    )
    keys_path = Path(
        "../../../../postgres/benchmark/tpch/legacy_scale_1/params_default/extract_gprom/keys.json"
    )
    parser.add_argument(
        "--keys",
        required=False,
        default=keys_path,
    )
    # Useful for debugging.
    add_gprom_candidates(parser)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None
    subdirs = config["subdirs"]
    assert len(subdirs) == 1
    gprom_config_file = Path(parsed.test_root) / (parsed.dir) / "config_gprom.json"
    gprom_config = json_read_file(gprom_config_file)
    assert gprom_config is not None

    query_class = QuerySpec
    if parsed.keys_mode:
        query_class = QueryLimitQuerySpec
    # Figure out which queries to include now.
    dir_queries = []
    cleaned_dir = Path(parsed.dir).name
    assert "/" not in cleaned_dir
    subdir_queries = []
    query_repr = config["queries"]
    key_file = json_read_file(parsed.keys)
    for query_name in parse_queries(query_repr):
        print(query_name)
        query_name = str(query_name)
        original_query_name = query_name
        if query_name not in gprom_config:
            query_name = query_name.rjust(2, "0")
            if query_name not in gprom_config:
                continue
        g_config_item = gprom_config[query_name]
        subdir_queries.extend(
            make_gprom_query(
                original_query_name, parsed.mode, g_config_item, query_class, key_file[original_query_name]
            )
        )
    dir_queries.append(QueryDirectory(dir_name=cleaned_dir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    result_prefix = result["prefix"]
    result["prefix"] = f"{result_prefix}_{cleaned_dir}"
    result["extras"] = dict(config=parsed.config)
    result["call_options"] = gprom_config["call_mode"]
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
