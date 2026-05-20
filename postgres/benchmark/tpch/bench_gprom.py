import argparse
from pathlib import Path

from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    add_gprom_candidates,
    infer_gprom_candidates,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import RunParams


def make_base_query(query_name: str):
    return Query(query_name=query_name, spec=QuerySpec(base="base.sql", key="base"))


def make_gprom_query(query_name: str, gprom_mode: str, gprom_config: dict):
    valid_specs = infer_gprom_candidates(gprom_mode, gprom_config)
    return [
        Query(
            query_name=query_name,
            spec=QuerySpec(base=f"{spec.safe_key()}.sql", key=spec.safe_key()),
        )
        for spec in valid_specs
    ]


def main():
    benchmark = GenericBenchmark("tpch-driver-gprom")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-t_root", "--test_root", required=True)
    parser.add_argument("--dir", required=True)
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

    # Figure out which queries to include now.
    dir_queries = []
    cleaned_dir = Path(parsed.dir).name
    assert "/" not in cleaned_dir
    subdir_queries = []
    query_repr = config["queries"]
    if isinstance(query_repr, str):
        query_repr = eval(query_repr)
    for query_name in query_repr:
        print(query_name)
        query_name = str(query_name)
        original_query_name = query_name
        if query_name not in gprom_config:
            query_name = query_name.rjust(2, "0")
            if query_name not in gprom_config:
                continue
        g_config_item = gprom_config[query_name]
        subdir_queries.extend(
            make_gprom_query(original_query_name, parsed.mode, g_config_item)
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
