from itertools import product
import json
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
        json_read_file(file.replace("%d_%d", f"{a}_{b}"))
        for a, b in product(first_iter, second_iter)
    ]


def traceprov_assert_safe_run(cmd: str):
    print("Running: ", cmd)
    assert os.system(cmd) == 0
    return 0
