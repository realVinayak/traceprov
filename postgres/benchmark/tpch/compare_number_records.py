import argparse
import json
from typing import Any

COUNT_PREFIX = "traceprov_infer_count_"


def extract_count(extras: list[dict[str, Any]]):
    extras = extras[0]
    result = []
    for key, value in extras.items():
        if key.startswith(COUNT_PREFIX):
            result.append(value["captured"][0][0])
    return result


def main():
    prog = argparse.ArgumentParser("compare-num-records")
    prog.add_argument("-n", "--new", required=True)
    prog.add_argument("-o", "--old", required=True)

    parsed = prog.parse_args()
    with open(parsed.new) as f:
        new_result = json.loads(f.read())

    with open(parsed.old) as f:
        old_result = json.loads(f.read())

    normalized_new_result = {
        key: {
            query_num: extract_count(q_result["traceprov"]["extras"])
            for query_num, q_result in result.items()
        }
        for (key, result) in new_result["result"].items()
    }

    normalized_old_result = {
        param: {q: q_results[0][-1] for (q, q_results) in result.items()}
        for param, result in old_result.items()
    }

    print(normalized_new_result)
    print(normalized_old_result)
    print(json.dumps(normalized_new_result) == json.dumps(normalized_old_result))


if __name__ == "__main__":
    main()
