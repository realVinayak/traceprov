import argparse
import json
import os
import subprocess
from typing import NamedTuple
import sys
import uuid
from run_with_timeout import run_with_timeout

SPECIAL_FILES = ["AUTO_TRACEPROV_TIME", "AUTO_TRACEPROV_MAT"]


class Options(NamedTuple):
    config: str
    database: str


def run_query(path, db, query, timeout):
    # run_bench_process.py -f AUTO_TRACEPROV_TIME -q 01 -o /tmp/rand.out -db microbench_agg_0

    #with open(path) as f:
    #    normal_sql = f.read()
    #    new_sql = f"set statement_timeout='{timeout}s';\n" + normal_sql

    #new_sql_file = f"/tmp/{uuid.uuid4()}.sql"
    #with open(new_sql_file, "w") as f:
    #    f.write(new_sql)

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
        '-t',
        str(timeout)
    ]
    print("running: ", args)

    try:
        run_with_timeout((args), timeout_sec=timeout)
        try:
            with open(file) as f:
                result = float(f.read())
        except:
            result = None
    except subprocess.TimeoutExpired:
        result = None

    return result


def run_subdir(subdir: str, config, iters: int, db: str):
    subdir_result = {}

    query_num = config["query_id"]
    make_path = lambda path: (
        path if path in SPECIAL_FILES else f"{subdir}/{query_num}/{path}"
    )

    for query_type in config["queries"]:
        base_query = query_type["base"]
        path = make_path(base_query)

        key = query_type["key"]

        query_type_results = dict(base=[], materialize=[], infer=[])

        for i in range(iters):

            os.system(
                f'echo "select reinit_state();" | PGPASSWORD=postgres psql -U postgres {db}'
            )
            result = run_query(path, db, query_num, config["timeout"])

            print("on index: ", i)
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

            time_query = query_type.get("time")
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
        subdir_results = run_subdir(subdir, config, iters, parsed.database)
        results[subdir] = subdir_results

    print(results)

    
    with open('microbench_agg_result.json', 'w') as f:
        f.write(json.dumps(results))

if __name__ == "__main__":
    main()
