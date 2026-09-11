import argparse
import os
from pathlib import Path

from traceprovpy.tools.file_utils import just_read, traceprov_assert_safe_run

import re
import glob

import duckdb

LINE_REG = r"^\d+\.(.*):\s*$"


def strip_pound(val: str):
    return val.split("#")[0].strip()


DB_FILE_MAPPING = dict(
    duckdb=["result"], postgres=["main_result", "benchmarks_params", "stats"]
)


def get_qualified_names(dbname, result_dir: Path, prefix: str):
    res_dirs = DB_FILE_MAPPING[dbname]
    return (
        [result_dir / f"{file}.json" for file in res_dirs],
        [f"{prefix}_{file}.json" for file in res_dirs],
    )


def extract_version(file_name: str):
    connection = duckdb.connect(file_name)
    cursor = connection.cursor()
    cursor.execute("select max(version) from dumped;")
    result = cursor.fetchall()
    max_value = result[0][0]
    cursor.close()
    connection.close()
    print(f"Max value of {file_name} -> {max_value}")
    return max_value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--notes", required=True)
    parser.add_argument("--extract_dir", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--old_duckdb_dir", required=False)
    parser.add_argument(
        "--dry_run", action=argparse.BooleanOptionalAction, default=False
    )
    parsed = parser.parse_args()
    start_version = 0
    if parsed.old_duckdb_dir is not None:
        # need to run the version query on all the old duckdb dirs to figure out the next
        # version to use.
        duckdb_files = glob.glob(f"{parsed.old_duckdb_dir}/*.db")
        assert len(duckdb_files) != 0, "Expected to find at least some duckdb files."
        for duckdb_file in duckdb_files:
            file_version = extract_version(duckdb_file)
            assert file_version is not None and file_version > 0
            start_version = max(start_version, file_version)
    start_version += 1
    print("Using start version: ", start_version)
    if parsed.dry_run:
        print("Dry run mode")
        return
    nodes_contents = parsed.notes
    extract_dir = Path(parsed.extract_dir)
    exp_notes = just_read(nodes_contents)
    exp_notes_split = exp_notes.splitlines()
    match_count = 0
    match_trimmed_dir = []
    for line in exp_notes_split:
        match = re.match(LINE_REG, line)
        if match is None:
            continue
        match_count += 1
        groups = match.groups()
        # print(line, groups)
        assert len(groups) == 1
        match_trimmed: str = groups[0].strip()
        print(match_trimmed)
        match_trimmed_dir.append(strip_pound(match_trimmed.lower()))
    pending = None
    file_dirs = []
    for line in exp_notes_split:
        stripped = line.strip()
        if stripped.startswith("#"):
            continue
        if len(stripped) == 0:
            continue
        if re.match(LINE_REG, line):
            continue
        if stripped == "```":
            if pending is None:
                pending = []
                continue
            file_dirs.append(pending)
            pending = []
            continue
        assert pending is not None
        pending.append(stripped)
    print("Res: ", match_count)
    for file_dir in file_dirs:
        if len(file_dir) == 0:
            continue
        print("file dir: ")
        print("\n".join(file_dir))
    file_dirs_filt = [l for l in file_dirs if len(l) > 0]
    print(len(file_dirs_filt))
    assert len(file_dirs_filt) == len(match_trimmed_dir)
    match_trimmed_nice = [mtd.split(" ") for mtd in match_trimmed_dir]
    print(match_trimmed_nice)
    total_length = len(str(len(file_dirs_filt)))
    for raw_g_idx, (current_file_dirs, mtd_elem) in enumerate(
        zip(file_dirs_filt, match_trimmed_nice, strict=True)
    ):
        raw_g_idx += start_version
        g_index = str(raw_g_idx).rjust(total_length, "0")
        db, sf_str, system, mode = tuple(mtd_elem)
        sf_str_split: str = sf_str.split("=")[1]
        sf_label = sf_str_split.rjust(2, "0")
        if db == "newduckdb":
            true_db = "duckdb"
        else:
            true_db = db
        core_result_dir = extract_dir / f"{true_db}_results_sf_{sf_label}_extract"
        assert core_result_dir.exists(), f"Expected {core_result_dir} to exist"
        terminal_names = [Path(cfd).name for cfd in current_file_dirs]
        final_dir: Path = Path(parsed.out_dir) / db / f"sf_{sf_label}" / mode / system
        os.makedirs(final_dir, exist_ok=True)
        for terminal_name in terminal_names:
            qns, targets = get_qualified_names(
                true_db,
                core_result_dir / "results" / terminal_name,
                f"g_{g_index}_{terminal_name}",
            )
            # print(qns, targets)
            for qn, target in zip(qns, targets):
                tt = final_dir / target
                print(f"cp {qn.absolute().as_posix()} {tt.absolute().as_posix()}")
                traceprov_assert_safe_run(
                    f"cp {qn.absolute().as_posix()} {tt.absolute().as_posix()}"
                )

        # os.makedirs(file_dirs, exist_ok=True)
    # print("file dirs", file_dirs)


if __name__ == "__main__":
    main()
