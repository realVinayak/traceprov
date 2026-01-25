# takes list of resut dirs, and asserts same infer count...
import argparse
from functools import reduce
import glob
import json
from pathlib import Path

from traceprovpy.tools.plot_utils import BenchmarkPlot


def assert_all_distinct(in_list):
    assert len(set(in_list)) == len(in_list)
    return in_list


def gen_dict(first, second):
    return {key: value for (key, value) in zip(first, second)}


def merge_qdata(previous, current):
    if len(previous) == 0:
        return {idx: [idx_value] for (idx, idx_value) in current.items()}

    # it shouldn't be possible to not find the key before.
    # so that's why no .get()
    return {idx: [*previous[idx], idx_value] for (idx, idx_value) in current.items()}


def merge(previous: dict, current: dict):
    return {
        **previous,
        **{
            qnum: merge_qdata(previous.get(qnum, {}), qdata)
            for (qnum, qdata) in current.items()
        },
    }


def main():
    parser = BenchmarkPlot("infer_count")
    parsed = parser.parser.parse_args()
    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            with open(path) as f:
                result = json.loads(f.read())
            all_results.append(result)
    print(len(all_results))
    mapped = [
        {
            qnum: gen_dict(
                assert_all_distinct(
                    [single["infer_id"] for single in qdata["result"]["infer_results"]]
                ),
                [
                    single["infer_out"][1]["row_count"]
                    for single in qdata["result"]["infer_results"]
                ],
            )
            for (qnum, qdata) in result.items()
        }
        for result in all_results
    ]
    reduced = reduce(merge, mapped, dict())
    for query, query_data in reduced.items():
        for idx, idx_values in query_data.items():
            print(query, idx, idx_values)
            if len(set(idx_values)) > 1:
                print("got different at: ", query, idx)


if __name__ == "__main__":
    main()
