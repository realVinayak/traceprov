import argparse
import os
from typing import NamedTuple
import time


class Options(NamedTuple):
    file: str
    q: str
    outfile: str
    db: str


def resolve_filename(file_name: str, query_num: str):
    # We've some
    if file_name == "AUTO_TRACEPROV_TIME":
        return f"templates/traceprov/{query_num}.infertime.sql"

    if file_name == "AUTO_TRACEPROV_MAT":
        return f"templates/traceprov/{query_num}.materialize.sql"

    return file_name


def main():
    parser = argparse.ArgumentParser(prog="aggregation-microbench-proc")
    parser.add_argument("-f", "--file", required=True, type=str)
    parser.add_argument("-q", "--q", required=True, type=str)
    parser.add_argument("-out", "--outfile", required=True, type=str)
    parser.add_argument("-db", "--db", required=True, type=str)

    parsed: Options = parser.parse_args()
    filename = resolve_filename(parsed.file, parsed.q)

    is_time = parsed.file == "AUTO_TRACEPROV_TIME"

    if is_time:
        os.system(
            f"PGPASSWORD=postgres psql -U postgres {parsed.db} -f {filename} --tuples > /tmp/time.out"
        )
        with open("/tmp/time.out") as f:
            computed_time = int(f.read().strip().replace("\n", "")) / 1000
    else:
        start = time.perf_counter()
        os.system(
            f"PGPASSWORD=postgres psql -U postgres {parsed.db} -f {filename} --tuples > /dev/null"
        )
        end = time.perf_counter()
        computed_time = end - start

    with open(parsed.outfile, "w") as f:
        f.writable(str(computed_time))


if __name__ == "__main__":
    main()
