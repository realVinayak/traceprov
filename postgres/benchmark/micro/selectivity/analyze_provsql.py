import glob
import json
from traceprovpy.tools.plot_utils import BenchmarkPlot, Plotable


def main():
    bench_plotter = BenchmarkPlot("selectity_microbenchmark")
    parsed = bench_plotter.parser.parse_args()
    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            is_traceprov = "traceprov" in path
            with open(path) as f:
                main_result = json.loads(f.read())
            print("checking:", path, is_traceprov)
            # bench_plotter.sanity_checks(main_result)
            all_results.append(rename(is_traceprov, main_result["result"]))


def rename(is_traceprov, input_result):
    return {
        dir_name: {
            f"{'provsql' if not is_traceprov else 'traceprov'}_{key}": value["base"]
            for (key, value) in dir_results["predicate_pre"].items()
        }
        for (dir_name, dir_results) in input_result.items()
    }


if __name__ == "__main__":
    main()
