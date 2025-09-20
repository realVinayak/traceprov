import argparse
import json
import os
import subprocess
from typing import NamedTuple
import sys
import uuid

SPECIAL_FILES = ["AUTO_TRACEPROV_TIME", "AUTO_TRACEPROV_MAT"]


class Options(NamedTuple):
    config: str
    database: str


def run_query(path, db, query, timeout):
    # run_bench_process.py -f AUTO_TRACEPROV_TIME -q 01 -o /tmp/rand.out -db microbench_agg_0
    file = f"/tmp/{uuid.uuid4()}.txt"

    args = [
        sys.executable,
        "run_bench_process.py",
        "-f",
        path,
        "-q",
        query,
        "-o",
        file,
        "-db",
        db,
    ]

    try:
        subprocess.run(args, timeout=timeout)
        with open(file) as f:
            result = float(f.read())
    except subprocess.TimeoutExpired:
        result = None

    return result


def run_subdir(subdir: str, config, iters: int, db: str):
    subdir_result = {}

    query_num = config["query_id"]
    make_path: lambda path: (
        path if path in SPECIAL_FILES else f"{subdir}/{query_num}/{path}"
    )

    for query_type in config["queries"]:
        base_query = query_type["base"]
        path = make_path(base_query)

        key = query_type["key"]

        query_type_results = dict(base=[], materialize=[], infer=[])

        for i in range(iters):

            result = run_query(path, db, query_num, config["timeout"])

            if result is None:
                break

            if i < config["throwaway"]:
                continue

            query_type_results["base"].append(result)

            materialize_query = query_type.get("materialize")
            if materialize_query is None:
                continue

            mat_result = run_query(
                make_path(materialize_query), db, query_num, config["timeout"]
            )

            if mat_result is None:
                continue

            query_type_results["materialize"].append(mat_result)

            time_query = query_type.get("materialize")
            if time_query is None:
                continue

            time_query_result = run_query(
                make_path(time_query), db, query_num, config["timeout"]
            )

            if time_query_result is None:
                continue

            query_type_results["infer"].append(time_query_result)

        subdir_result[key] = query_type_results

    return subdir_result


def main():
    parser = argparse.ArgumentParser(prog="aggregation-microbench")
    parser.add_argument("-cfg", "--config", required=True, type=str)
    parser.add_argument("-db", "--database", required=True, type=str)

    parsed: Options = parser.parse_args()

    with open(parsed.config) as pf:
        config = json.loads(pf.read())

    iters = config["repeat"] + config["throwaway"]

    results = {}

    for subdir in config["subdirs"]:
        current_result = results[subdir]
        subdir_results = run_subdir(subdir, config, iters, parsed.database)
        current_result[subdir] = subdir_results

    print(subdir_results)


if __name__ == "__main__":
    main()
