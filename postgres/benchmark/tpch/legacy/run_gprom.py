import subprocess
import sys
import json
import time
from utils import safe_run, get_total_logged_records


def run_query(db_name, query_file):
    start = time.perf_counter()
    safe_run(
        f"PGPASSWORD=postgres psql -U postgres {db_name} -f {query_file} > /tmp/gprom_temp.out"
    )
    end = time.perf_counter()

    duration = end - start
    num_records = get_total_logged_records("/tmp/gprom_temp.out")
    return dict(duration=duration, num_records=num_records)


if __name__ == "__main__":
    db_name = sys.argv[1]
    query_file = sys.argv[2]
    json_file = sys.argv[3]
    obj = run_query(db_name, query_file)

    with open(json_file, "w") as jf:
        jf.write(json.dumps(obj))
