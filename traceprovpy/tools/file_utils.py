from itertools import product
import json
import math
import os
from pathlib import Path


def safe_file(func):
    def _func(file, *args, **kwargs):
        if file is None:
            return None
        return func(file, *args, **kwargs)

    return _func


# misc wrappers to simplify stuff.
# TODO: Put them somewhere more useful...
def just_read(file: str | Path):
    with open(file) as f:
        result = f.read()
    return result


@safe_file
def just_write(file: str | Path, contents: str):
    with open(file, "w") as f:
        f.write(contents)

    # always return the safest path possible.
    return Path(file).as_posix()


@safe_file
def json_read_file(file: str, strict_file: bool = False):
    try:
        with open(file) as f:
            json_content = json.loads(f.read())
        return json_content
    except Exception as e:
        if strict_file:
            raise
        try:
            return just_read(file)
        except:
            return None


def json_read_files(files: list[str]):
    return list(map(json_read_file, files))


@safe_file
def json_read_iters(file: str, iters: int):
    assert "%d" in file
    return [json_read_file(file.replace("%d", iter)) for iter in map(str, range(iters))]


@safe_file
def json_read_two_iters(file: str, first_iter: list[int], second_iter: list[int]):
    assert "%d_%d" in file
    return [
        json_read_file(file.replace("%d_%d", f"{a}_{b}"), True)
        for a, b in product(first_iter, second_iter)
    ]


def traceprov_assert_safe_run(cmd: str):
    print("Running: ", cmd)
    assert os.system(cmd) == 0
    return 0


def null_safe(in_list: list):
    return [0 if i is None else i for i in in_list]


def get_total_iters(config: dict):
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    return total_iters


TP_OUT_ID_TICKER = "%OUT_ID%"


def get_slowdown_cats(slowdown_data, break_points):
    slowdown_data = sorted(slowdown_data, key=lambda tup: tup[1])
    cats = []
    for _ in range(len(break_points) + 1):
        cats.append([])
    for query, slowdown in slowdown_data:
        for break_point_idx, break_point in enumerate(break_points):
            if slowdown <= break_point:
                break
        else:
            break_point_idx = len(break_points)
        cats[break_point_idx].append(query)
    max_slowdown = slowdown_data[-1][1]
    assert max_slowdown > 0
    break_points.append(round(max_slowdown, 2))
    print("CATS: ", cats, break_points)
    return cats


def get_min_max_categories(slowdown_data, break_points):
    broken_cats = get_slowdown_cats(slowdown_data, break_points)
    slowdown_data = sorted(slowdown_data, key=lambda tup: tup[1])
    min_max_pairs = []
    min_value, max_value = slowdown_data[0][1], break_points[-1]
    if len(broken_cats) == 1:
        min_max_pairs = [[min_value, max_value]]
    for cat_idx, _ in enumerate(broken_cats):
        current_max_value = break_points[cat_idx]
        current_min_value = min_value if cat_idx == 0 else break_points[cat_idx - 1]
        min_max_pairs.append((current_min_value, current_max_value))
    return (slowdown_data[0], slowdown_data[-1]), min_max_pairs, broken_cats


def get_breakpoint_with_min_max(min_max_pairs, add_percent=True):
    labels = []
    round_precision = 0 if add_percent else 2
    for _b_idx, (_min_value, _max_value) in enumerate(min_max_pairs):
        suffix = rf"\%" if add_percent else rf"\times"
        upper = rf"$\leq {round(_max_value, round_precision)}{suffix}$"
        lower = rf"$> {round(_min_value, round_precision)}{suffix}$"
        labels.append(f"{lower} & {upper}")
        continue
    assert len(labels) == len(min_max_pairs)
    return labels


def get_breakpoint_labels(breakpoints, add_percent=True):
    labels = []
    for _b_idx, _b_val in enumerate(breakpoints):
        suffix = rf"\%" if add_percent else rf"\times"
        current = rf"$\leq {_b_val}{suffix}$"
        if _b_idx == 0:
            labels.append(current)
            continue
        if add_percent:
            lower = f"$> {breakpoints[_b_idx-1]}\%$"
        else:
            lower = rf"$> {breakpoints[_b_idx-1]}\times$"
        labels.append(f"{lower} & {current}")
        continue
    assert len(labels) == len(breakpoints)
    return labels


def try_leq_1(in_int):
    return str(in_int)


def get_nice_num(in_int):
    if in_int < 1:
        return str(in_int)
    if in_int < 1_000:
        return str(int(in_int))
    if in_int < 1_000_000:
        return f"{int(in_int / 1_000)} K"
    if in_int < 1_000_000_000:
        return f"{int(in_int / 1_000_000)} M"
    if in_int < 1_000_000_000_000:
        return f"{int(in_int / 1_000_000_000)} B"
    assert 0, in_int


# ALL the tmp files get written to this directory.
# This is done because usually we'd prefer /tmp/traceprov
# But, for debugging ./tmp is also nice.
# most experiments will make do with the default
# benefit is also that it is simpler to delete them.
_GLOBAL_TMP_DIR = "/tmp/traceprov/"


def make_tmp_file(later_path: "str"):
    assert Path(_GLOBAL_TMP_DIR).exists(), f"Expected {_GLOBAL_TMP_DIR} to exist!"
    return f"{_GLOBAL_TMP_DIR}/{later_path}"


def set_tmp_file(new_path: "str"):
    global _GLOBAL_TMP_DIR
    os.makedirs(new_path, exist_ok=True)
    _GLOBAL_TMP_DIR = new_path


def get_tmp_file():
    global _GLOBAL_TMP_DIR
    os.makedirs(_GLOBAL_TMP_DIR, exist_ok=True)
    return _GLOBAL_TMP_DIR


def run_query(cursor, query):
    cursor.execute(query)
    return cursor.fetchall()
