import argparse
import os
from pathlib import Path
from typing import NamedTuple

from traceprovpy.tools.extract_query_results import dump_rows_list, handle_duckdb_result
from traceprovpy.tools.file_utils import extract_from_notes, json_read_file, just_read


class MockKey(NamedTuple):
    num_rows: int
    group_num: int


QUERY_ID = ("query_num_num_rows", "query_num_group_num")


def _pop_key(in_dict: dict):
    si = in_dict["result"].pop("sample_inference")
    in_dict["sample_inference_result"] = si
    return in_dict


def map_traceprov_result(group_num, current_file_content):
    combined_results = current_file_content["results"]["result"]

    mapped_result = {
        MockKey(num_rows=result["dir"], group_num=group_num): _pop_key(result)
        for result in combined_results
    }
    assert len(mapped_result) == len(
        combined_results
    ), "Expected keys to be strictly unique anyways!"
    return dict(
        results=mapped_result, call_options=current_file_content["call_options"]
    )


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", required=False)
    parser.add_argument("--notes", required=True)
    parser.add_argument(
        "--dry_run", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--out_dir", required=False, default="./tmp/")
    parsed = parser.parse_args()
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)

    notes = just_read(parsed.notes)
    assert notes is not None
    dirs, file_dirs = extract_from_notes(notes)
    print(dirs, file_dirs)

    if parsed.dry_run:
        return

    assert parsed.dir is not None

    root_dir = Path(parsed.dir)
    assert root_dir is not None

    all_rows = []
    version_number = 0
    for current_dir, current_file_dirs in zip(dirs, file_dirs, strict=True):
        backend_system, group_num_str, db_system = tuple(current_dir)
        group_num = int(group_num_str.split("=")[1])

        for current_file in current_file_dirs:
            version_number += 1
            current_file_path = root_dir / str(current_file) / "result.json"
            assert current_file_path.exists()
            current_file_content = json_read_file(current_file_path)
            true_result = None
            if db_system == "traceprov":
                true_result = map_traceprov_result(group_num, current_file_content)
            elif db_system == "smokedduck":
                # this works fine for now.
                map_smokedduck_result = map_traceprov_result
                true_result = map_smokedduck_result(group_num, current_file_content)
            else:
                raise Exception(f"Got unhandled system: {db_system}")
            if true_result is None:
                continue
            handle_duckdb_result(
                db_system,
                "offset",
                true_result,
                root_dir / str(current_file),
                all_rows,
                version_number,
            )
    for row in all_rows:
        assert hasattr(row, "query_num")
        row.query_num = row.query_num._asdict()

    dump_rows_list("0", "duckdb", "offset", all_rows, out_dir, QUERY_ID)


if __name__ == "__main__":
    run()
