import argparse
from pathlib import Path

from traceprovpy.tools.benchmark import GenericBenchmark, Query, QueryDirectory
from traceprovpy.tools.duckdb_inference import ExtractJsonGraphQuerySpec
from traceprovpy.tools.run_with_timeout import TP_SKIPPABLE_OPTION, RunParams


def make_query(query_name: str, source_dir: Path, target_dir: Path):
    return Query(
        query_name=query_name,
        spec=ExtractJsonGraphQuerySpec(
            base=TP_SKIPPABLE_OPTION,
            key="extract_json_graph",
            extra_options=dict(
                source_graph_path=source_dir / query_name / "graph.bin",
                target_graph_path=target_dir / query_name / "graph.bin",
            ),
        ),
    )


def main():
    benchmark = GenericBenchmark("tpch-json-graphs")
    parser = argparse.ArgumentParser()
    parser.add_argument("--source_dir", required=True)
    parser.add_argument("--target_dir", required=True)

    parsed, _ = parser.parse_known_args()
    queries = []
    for query in map(str, range(1, 23)):
        queries.append(
            make_query(query, Path(parsed.source_dir), Path(parsed.target_dir))
        )
    dir_queries = [QueryDirectory(dir_name="params_default", queries=queries)]
    result = benchmark.run_from_argparse(dir_queries, RunParams(repeat=1, throwaway=0))
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
