from traceprovpy.tools.plot_utils import BenchmarkPlot, Plotable
import glob
import json

COMPRESSIONS = ['uncompressed', "snappy", "gzip","zstd", "brotli", "lz4", "lz4_raw" ]

def assert_len_one(array):
    assert len(array) == 1
    if isinstance(array[0], list):
        return assert_len_one(array[0])
    else:
        return array[0]


def extract_path(path: str):
    path_split = path.split("/")
    assert len(path_split) == 4
    path_name = path_split[2]
    assert "local_test_duckdb_analyze_" in path_name
    path_name = path_name.replace("local_test_duckdb_analyze_", "")


def main():
    bench_plotter = BenchmarkPlot("duckdb_inference")
    parsed = bench_plotter.parser.parse_args()
    for file in parsed.files:
        print(file)
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            extract_path(path)
            with open(path) as f:
                main_result = json.loads(f.read())


if __name__ == "__main__":
    main()
