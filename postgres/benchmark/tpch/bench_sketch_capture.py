import argparse
import glob
import json
from pathlib import Path

from traceprovpy.tools.benchmark import (
    DropTable,
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
    SketchValidationQuerySpec,
    ValidationQuerySpec,
)
from traceprovpy.tools.benchmark_utils import (
    TRACEPROV_PREPARE_INFER_SPEC,
    make_traceprov_drop_table_extra,
)
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    MakeTraceProv,
    MatMaterialize,
    ReplaceBucket,
    RunParams,
)
from traceprovpy.tools.traceprov_extra_func import (
    TRACEPROV_LAYERS_TO_DERIVE_KEY,
    TRACEPROV_MATERIALIZE_LAYER_KEY,
)


def get_gprom(table: str):
    return f"g_capture_{table}"


def get_tp_table(table: str):
    return f"traceprov_infer_capture_{table}"


def make_query(query_name: str, partition_count: str, mat: bool = False):
    preprocessor = []
    extras = []
    if mat:
        table_name = get_gprom(query_name)
        preprocessor = [MatMaterialize(table_name)]
        # extras = [make_traceprov_drop_table_extra(table_name)()]
    return Query(
        query_name=partition_count,
        # In this case, use the query name itself as the key for the result.
        spec=QuerySpec(
            base=f"q{query_name}.sql",
            key=query_name,
            extras=extras,
            preprocess=preprocessor,
        ),
    )


def get_base_query(query_name):
    return f"$ROOT/../nor/q{query_name}.sql"


def make_base_query(query_name: str, partition_count: str):
    return Query(
        query_name=partition_count,
        # In this case, use the query name itself as the key for the result.
        spec=QuerySpec(base=get_base_query(query_name), key=f"base_{query_name}"),
    )


def make_traceprov_query(
    query_name: str,
    partition_count: str,
    partition_spec: dict[str, list[int]],
    sketch_queries: list[str],
    layers_to_derive=[],
    mat: bool = False,
):
    base_file = get_base_query(query_name)
    extras: list[ExtraQuery] = [TRACEPROV_PREPARE_INFER_SPEC()]
    mat_extras = []
    for s_query in sketch_queries:
        preprocess = []
        if mat:
            table_name = get_tp_table(query_name)
            preprocess = [MatMaterialize(table_name)]
            mat_extras.append(make_traceprov_drop_table_extra(table_name)())
        extras.append(
            ExtraQuery(
                label="traceprov_capture_sketch",
                query=s_query,
                runs_after_base=True,
                preprocess=[ReplaceBucket(partition_spec), *preprocess],
                capture_output=False,
            )
        )

    extras = [*mat_extras, *extras]

    return Query(
        query_name=partition_count,
        spec=QuerySpec(
            base=base_file,
            key=f"traceprov_{query_name}",
            preprocess=[MakeTraceProv()],
            extras=extras,
            extra_options={
                TRACEPROV_LAYERS_TO_DERIVE_KEY: layers_to_derive,
                TRACEPROV_MATERIALIZE_LAYER_KEY: False,
            },
        ),
    )


def make_validate_query(query_name: str):
    return Query(
        query_name=query_name,
        spec=SketchValidationQuerySpec(
            base=get_gprom(query_name),
            key=f"VALIDATION_{next(gen)}",
            materialize=get_tp_table(query_name),
        ),
    )


def get_next():
    i = 0
    while True:
        i += 1
        yield i


gen = get_next()


def make_drop_table(query: str):
    tables = [get_gprom(query), get_tp_table(query)]
    return Query(
        query_name=query,
        spec=DropTable(
            base=":".join(tables),
            key=f"DROP_TABLES_{next(gen)}",
        ),
    )


derivable_rows = {
    "2": {"layers_used": [1, 2]},
    "18": {"layers_used": [1, 2]},
    "20": {"layers_used": [1, 2]},
}


def main():
    benchmark = GenericBenchmark("tpch-driver-muller")
    parser = argparse.ArgumentParser(prog="tpch-driver")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("--derive_config", required=True, type=str)
    parser.add_argument("-t_root", required=True, type=str)
    parser.add_argument(
        f"--mat",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parsed, _ = parser.parse_known_args()
    curr = Path(parsed.t_root)

    config = json_read_file(parsed.config)
    derive_config = json_read_file(parsed.derive_config)

    assert config and derive_config

    partitions = config["partitions"]
    queries = []
    subdir = config["subdir"]
    derive_config = {**derive_config, **derivable_rows}
    for partition in partitions:
        partition_queries = partition["queries"]
        partition_count = partition["count"]
        for query in partition_queries:
            q_dir = curr / subdir / partition_count
            query_buckets = q_dir / f"q{query}.json"
            assert (
                query_buckets.exists()
            ), f"Expected {query_buckets.absolute()} to exist!"
            query_bucket_content = json_read_file(query_buckets)
            queries.append(make_drop_table(query))
            queries.append(make_base_query(query, partition_count))
            queries.append(make_query(query, partition_count, parsed.mat))
            paths = list(glob.glob(f"{q_dir.as_posix()}/q{query}_traceprov_layer*.sql"))
            assert len(paths) > 0
            leafed = [Path(path).parts[-1] for path in paths]
            print("LEAFS", leafed)
            queries.append(
                make_traceprov_query(
                    query,
                    partition_count,
                    query_bucket_content,
                    leafed,
                    layers_to_derive=derive_config[query]["layers_used"],
                    mat=parsed.mat,
                )
            )
            if parsed.mat:
                queries.append(make_validate_query(query))
                queries.append(make_drop_table(query))

    query_dir = QueryDirectory(dir_name=subdir, queries=queries)
    result = benchmark.run_from_argparse(
        [query_dir], RunParams(**config.get("runTimeOptions", {}))
    )
    if "extras" in result:
        raise Exception('Expected "extras" to be a reserved keyword.')

    # Also store the arguments from cmd line.
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
