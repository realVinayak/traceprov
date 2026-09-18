import argparse
from functools import reduce
import glob
from pathlib import Path

from traceprovpy.tools.extract_query_results import make_versioned_query
from traceprovpy.tools.file_utils import traceprov_assert_safe_run
from utils import DataOptions, get_options_split
import duckdb


class MockObject:
    db: str

    def __init__(self, db):
        self.db = db


def get_option_file_mapping(db_dir):
    db_files = glob.glob(f"{db_dir}/*.db")
    mapping = dict()
    for db_file in db_files:
        mock = MockObject(db_file)
        data_option = get_options_split(mock)
        assert (
            data_option not in mapping
        ), f"didn't expect to find {data_option} in mapping"
        mapping[data_option] = db_file
    return mapping


def reduce_to_list(mapping: list[dict]):
    def _reduce(prev: dict, curr: dict):
        return {
            **prev,
            **{key: [*prev.get(key, []), value] for (key, value) in curr.items()},
        }

    return reduce(_reduce, mapping, dict())


def make_backup_file(db_path: Path):
    copy_name = f"{db_path.name}.old"
    return db_path.parent / copy_name


def merge_files(files: list[str], data_option: DataOptions, out_dir: Path):
    out_db_path = out_dir / data_option.to_db()
    out_db_path = out_db_path.resolve()
    if out_db_path.exists():
        print("Using already existing file for merge result: ", out_db_path)
        copy_name = make_backup_file(out_db_path)
        # Only make the copy once in the beginning.
        if not copy_name.exists():
            traceprov_assert_safe_run(f"cp {out_db_path} {copy_name}")
    conn = duckdb.connect(out_db_path)
    cursor = conn.cursor()
    all_dbs = []
    for file_id, file in enumerate(files):
        in_db_path = Path(file)
        backup_file = make_backup_file(in_db_path)
        if backup_file.exists():
            # if there is a backup for this file, use it.
            # doing this allows the code to be indempotent
            print(f"Using backup for {in_db_path}->{backup_file}")
            in_db_path = backup_file
        # in this case, the input db is the output db.
        # so, don't need to attach here.
        if in_db_path.resolve() == out_db_path:
            all_dbs.append("main")
            continue
        schema_name = f"file_{file_id}"
        attach_query = f"attach '{in_db_path}' as {schema_name}"
        print("attach query: ", attach_query)
        cursor.execute(attach_query)
        all_dbs.append(schema_name)
    # now, all the input paths are attached.
    # need to just run the union query.
    union_query_list = [
        f"(select * from {attached_db}.dumped)" for attached_db in all_dbs
    ]
    union_query = " UNION ALL BY NAME ".join(union_query_list)
    create_query = (
        f"create or replace table main.dumped as (select * from ({union_query}))"
    )
    print("create query: ", create_query)
    cursor.execute(create_query)
    cursor.close()
    conn.close()


def main():
    parser = argparse.ArgumentParser("postprocess_tpch_db")
    # takes multiple dirs and merges the results into one.
    # currently, generates a new dir in doing do.
    parser.add_argument("--dir", required=True, nargs="+")
    parser.add_argument("--out_dir", required=False)

    parsed = parser.parse_args()
    is_dummy = parsed.out_dir is None

    all_option_mapping = reduce_to_list(list(map(get_option_file_mapping, parsed.dir)))
    print(f"Using mapping: {all_option_mapping}")

    if is_dummy:
        return

    if parsed.out_dir == "0":
        parsed.out_dir = parsed.dir[0]

    parsed.out_dir = Path(parsed.out_dir)
    for _data_option, data_files in all_option_mapping.items():
        data_option: DataOptions = _data_option
        if len(data_files) > 1:
            # need to merge the dumped files first.
            merge_files(data_files, data_option, parsed.out_dir)

        data_file = parsed.out_dir / data_option.to_db()
        assert data_file.exists(), f"expected {data_file} to exist!"
        print("Using file for final version query: ", data_file)
        conn = duckdb.connect(data_file)
        cursor = conn.cursor()
        make_versioned_query(cursor, data_option.mode, data_option.get_db_name())
        cursor.close()
        conn.close()
    # db_dir = f"{parsed.dir}/"
    # db_files = glob.glob(f"{db_dir}/*.db")
    # assert len(db_files) > 0
    # for db_file in db_files:
    #     print(f"on file: {db_file}")
    #     mock = MockObject(db_file)
    #     data_option = get_options_split(mock)
    #     continue
    #     conn = duckdb.connect(db_file)
    #     cursor = conn.cursor()
    #     make_versioned_query(cursor, data_option.mode, data_option.get_db_name())
    #     cursor.close()
    #     conn.close()


if __name__ == "__main__":
    main()
