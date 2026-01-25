# runs a single query (with optional validation)
# done this way to make things more manageable.


import argparse
import json
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_with_timeout import DEFAULT_REPEAT, DEFAULT_THROWAWAY


# misc wrappers to simplify stuff.
# TODO: Put them somewhere more useful...
def just_read(file: str | Path):
    with open(file) as f:
        result = f.read()
    return result


def just_write(file: str | Path, contents: str):
    with open(file, "w") as f:
        f.write(contents)


def json_read_file(file: str):
    with open(file) as f:
        json_content = json.loads(f.read())
    return json_content


def json_read_iters(file: str, iters: int):
    assert "%d" in file
    return [json_read_file(file.replace("%d", iter)) for iter in map(str, range(iters))]


def make_dump_query(in_query: str, out_path: str):
    in_query = in_query.replace(";", "")
    dump_query = f"copy (select * from ({in_query}) f order by all) to '{out_path}' (header false)"
    return dump_query


def run_single(
    exe: Path,
    db: Path,
    query_num: str,
    base_root: Path,
    root: Path,
    spec_element: dict,
    use_optimized: bool = False,
    validate: bool = False,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    disable_col_opt: bool = False,
    materialize_infer: bool = False,
    pre_base: Path | None = None,
    run_inference: bool = True,
):
    if validate:
        materialize_infer = True
    # if validating, assert not using optimized, for now...
    # NOTE: supports the below now.
    # assert not validate or not use_optimized

    # bc those are postgres queries....
    base_sql = base_root / query_num / "base.sql"

    query_dir = root / query_num
    if use_optimized:
        captured_sql = query_dir / "capture_new.sql"
    else:
        captured_sql = query_dir / "capture.sql"

    min_layer_used = spec_element["min_local_used"]

    exec_str = exe.as_posix()

    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(), repeat=1, threads=1, i=(query_dir / pre_base).as_posix()
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    base_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=1,
        i=base_sql.as_posix(),
        time="/tmp/base_time.json",
        min_layer_number=min_layer_used,
        profile="/tmp/base_profile_%d.json",
    )

    traceprov_assert_safe_run(f"{exec_str} {base_options.serialize()}")
    base_result_time = json_read_file(base_options.time)
    base_profile_out = json_read_iters(base_options.profile, base_options.repeat)

    capture_options = base_options._replace(
        i=captured_sql.as_posix(),
        time="/tmp/capture_time.json",
        profile="/tmp/capture_profile_%d.json",
        disable_col_opt=disable_col_opt,
    )

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    capture_profile_out = json_read_iters(
        capture_options.profile, capture_options.repeat
    )

    infer_results = []
    if run_inference:
        for element in spec_element["elements"]:
            element_idx = element["idx"]
            if use_optimized:
                infer_path = query_dir / f"infer_{element_idx}_new.sql"
            else:
                infer_path = query_dir / f"infer_{element_idx}.sql"

            assert infer_path.exists()
            infer_path = infer_path.as_posix()
            if materialize_infer:
                contents = just_read(infer_path)
                table_name = f"LAYER_{element_idx}"
                infer_path = (
                    Path("/tmp/") / f"infer_{element_idx}_materialize.sql"
                ).as_posix()
                mat_contents = f"create or replace table {table_name} AS ({contents});"
                just_write(infer_path, mat_contents)

            infer_option = DuckDBDriverOptions(
                db=capture_options.db,
                repeat=1,
                threads=1,
                i=captured_sql,
                extra=infer_path,
                time=f"/tmp/infer_out_{element_idx}.json",
                min_layer_number=min_layer_used,
                disable_col_opt=capture_options.disable_col_opt,
            )
            traceprov_assert_safe_run(f"{exec_str} {infer_option.serialize()}")
            infer_results = [
                *infer_results,
                dict(infer_id=element_idx, infer_out=json_read_file(infer_option.time)),
            ]

    if validate:
        # need to dump base and infer result on provenance, and compare both.
        base_dump_path = Path("/tmp/") / "base_dump.csv"
        base_dump_query_path = Path("/tmp/") / "base_dump_query.sql"
        base_dump_query = make_dump_query(
            just_read(base_options.i), base_dump_path.as_posix()
        )
        just_write(base_dump_query_path, base_dump_query)

        capture_dump_path = Path("/tmp/") / "capture_dump.csv"
        if use_optimized:
            validate_query = query_dir / "validate_new.sql"
        else:
            validate_query = query_dir / "validate.sql"
        assert validate_query.exists()
        capture_dump_query_path = Path("/tmp/") / "capture_dump_query.sql"
        capture_dump_query = make_dump_query(
            just_read(validate_query), capture_dump_path.as_posix()
        )
        just_write(capture_dump_query_path, capture_dump_query)

        simple_option = lambda in_path: DuckDBDriverOptions(
            db=base_options.db,
            i=in_path.as_posix(),
            repeat=1,
        )

        traceprov_assert_safe_run(
            f"{exec_str} {simple_option(base_dump_query_path).serialize()}"
        )
        traceprov_assert_safe_run(
            f"{exec_str} {simple_option(capture_dump_query_path).serialize()}"
        )

        print(base_dump_path, base_dump_path.stat().st_size)
        print(capture_dump_path, capture_dump_path.stat().st_size)

        traceprov_assert_safe_run(
            f"diff {base_dump_path.as_posix()} {capture_dump_path.as_posix()}"
        )

    final_result = dict(
        base_time=base_result_time,
        base_profile=base_profile_out,
        capture_time=capture_result_time,
        capture_profile=capture_profile_out,
        infer_results=infer_results,
    )
    return final_result


def add_query_options(parser: argparse.ArgumentParser):
    parser.add_argument("--spec", required=True)
    parser.add_argument("--base_root", required=True)
    parser.add_argument("--root", required=True)
    parser.add_argument(
        "--optimized", action=argparse.BooleanOptionalAction, default=False
    )


def main():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--qnum", required=True)
    add_query_options(base_parser)
    base_parser.add_argument(
        "--validate", action=argparse.BooleanOptionalAction, default=False
    )
    base_parser.add_argument(
        "--mat_infer", action=argparse.BooleanOptionalAction, default=False
    )
    base_parser.add_argument(
        "--disable_duck_opt", action=argparse.BooleanOptionalAction, default=False
    )
    # Stuff to run just before the base..
    # Maybe handle this more nicely (as done in Postgres implementation)
    base_parser.add_argument("--pre_base", required=False)
    parsed = base_parser.parse_args()
    result = run_single(
        exe=Path(parsed.exe),
        db=Path(parsed.db),
        query_num=parsed.qnum,
        base_root=Path(parsed.base_root),
        root=Path(parsed.root),
        spec_element=json_read_file(parsed.spec)[parsed.qnum][0],
        use_optimized=parsed.optimized,
        validate=parsed.validate,
        disable_col_opt=parsed.disable_duck_opt,
        materialize_infer=parsed.mat_infer,
        # temp...
        iters=3,
        pre_base=Path(parsed.pre_base) if parsed.pre_base is not None else None,
    )
    with open("playground/out.json", "w") as f:
        f.write(json.dumps(result, indent=4))


if __name__ == "__main__":
    main()
