import argparse
import json


def extract(query_data: dict):
    return [
        json.loads(extra["traceprov_generic_derivation_spec"]["captured"][0][0])
        for extra in query_data["extras"]
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--result", required=True)
    parsed = parser.parse_args()

    with open(parsed.result) as f:
        result = json.loads(f.read())["result"]["params_default"]

    # print(result)

    result_mapped = {
        qnum: extract(
            query_data["traceprov_15_skippable"]
            if "traceprov_15_skippable" in query_data
            else query_data["traceprov"]
        )
        for (qnum, query_data) in result.items()
    }

    with open("spec.json", "w") as f:
        f.write(json.dumps(result_mapped))


if __name__ == "__main__":
    main()
