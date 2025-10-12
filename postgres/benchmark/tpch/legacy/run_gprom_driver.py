from collections import defaultdict
import subprocess
import sys
import json
import time
import uuid
from utils import THROWAWAY

TIMEOUT = 15 * 60
THROWAWAY = 0


def run_subprocess(db_name, query):
    file_name = f"/tmp/{uuid.uuid4()}.json"

    try:
        subprocess.run(
            [sys.executable, "run_gprom.py", db_name, query, file_name], timeout=TIMEOUT
        )
        with open(file_name) as f:
            result = json.loads(f.read())
    except subprocess.TimeoutExpired:
        result = None

    return result


def run():
    config_file = sys.argv[1]

    with open(config_file) as cf:
        config = json.loads(cf.read())

    test_queries = config["queries"]
    repeat = config["repeat"] + THROWAWAY
    test_dirs = config["subdirs"]
    db_name = config["db_name"]

    result_store = defaultdict(lambda: defaultdict(list))

    def _add_duration(test_dir, query, duration, previous):
        return {
            **previous,
            test_dir: {
                **previous.get(test_dir, {}),
                query: [*previous.get(test_dir, {}).get(query, []), duration],
            },
        }

    for test_dir in test_dirs:
        path = "/".join([*config_file.split("/")[:-1], test_dir, ""])

        for query in test_queries:

            for repetition in range(repeat):

                print(
                    "[GPROM] Running query:\t",
                    query,
                    "num",
                    repetition,
                    "dir:\t",
                    test_dir,
                )

                query_file_name = f"{path}gprom_materialized/{query}"
                result = run_subprocess(db_name, query_file_name)
                print(result)

                if result is None:
                    break

                # if we're going to throwaway, don't bother storing it
                if repetition < THROWAWAY:
                    continue

                result_store = _add_duration(test_dir, query, result, result_store)

    with open("gprom_result_store.json", "w") as grs:
        grs.write(json.dumps(result_store))


if __name__ == "__main__":
    run()
