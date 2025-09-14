import os
import re

SELECT_RE = r"SELECT\s*(\d*)"
THROWAWAY = 3


def get_total_logged_records(file="/tmp/temp.out"):
    with open(file) as tp:
        contents = tp.read()

    total = (int(val) for val in re.findall(SELECT_RE, contents))
    return list(total)


def safe_run(cmd):
    assert os.system(cmd) == 0
