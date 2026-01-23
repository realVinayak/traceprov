import argparse
import json
import statistics


def compute_median(result):
    return statistics.median(row.get("time") for row in result)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", required=True)
    parsed = parser.parse_args()
    with open(parsed.file) as f:
        results = json.loads(f.read())
    speced = {
        qnum: dict(
            base=compute_median(data["base"]), capture=compute_median(data["forward"])
        )
        for (qnum, data) in results.items()
    }
    slowdown = {
        qnum: 100 * ((data["capture"] / data["base"]) - 1)
        for qnum, data in speced.items()
    }
    print(slowdown)


if __name__ == "__main__":
    main()
