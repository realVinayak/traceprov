import argparse
from itertools import product
from unittest import result

from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    add_gprom_candidates,
    get_gprom_candidates,
    infer_gprom_candidates,
)
from traceprovpy.tools.extract_gprom_simple import GpromOptions
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
    parser.add_argument("-g_cfg", "--gprom_config", required=True, type=str)
    # Useful for debugging.
    add_gprom_candidates(parser)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None
    gprom_config = json_read_file(parsed.gprom_config)
    assert gprom_config is not None

    # Figure out which queries to include now.
    dir_queries = []

    for subdir in config["subdirs"]:
        subdir_queries = []
        query_repr = config["queries"]
        if isinstance(query_repr, str):
            query_repr = eval(query_repr)
        for query_name in query_repr:
            print(query_name)
            query_name = str(query_name)
            if query_name not in gprom_config:
                continue
            g_config_item = gprom_config[query_name]
            subdir_queries.extend(
                make_gprom_query(query_name, parsed.mode, g_config_item)
            )
        dir_queries.append(QueryDirectory(dir_name=subdir, queries=subdir_queries))

    result = benchmark.run_from_argparse(
        dir_queries, RunParams(**config.get("runTimeOptions", {}))
    )
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
