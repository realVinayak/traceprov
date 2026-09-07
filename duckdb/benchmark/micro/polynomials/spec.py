import argparse
import json
import os
from pathlib import Path

from traceprovpy.tools.file_utils import (
    just_read,
    just_write,
    traceprov_assert_safe_run,
)

ALL_SPEC = {
    "1": {
        "1000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"polynomial_table_control_0"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"skew_1_0_num_1000"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "5000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_5000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "10000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_10000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "50000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_50000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "100000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_100000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "500000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_500000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
        "1000000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_1000000"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null}],"opid":"3","extra":null}',
    },
    "2": {
        "1000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"}],"opid":"2","extra":null},{"name":"SEQ_SCAN ","children":[],"opid":"3","extra":"skew_1_0_num_1000"}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "5000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_5000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "10000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_10000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "50000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_50000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "100000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_100000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "500000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_500000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
        "1000000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_1000000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_0"}],"opid":"3","extra":null}],"opid":"4","extra":null}],"opid":"5","extra":null}',
    },
    "3": {
        "1000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_1"}],"opid":"2","extra":null},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"3","extra":"polynomial_table_control_0"},{"name":"SEQ_SCAN ","children":[],"opid":"4","extra":"skew_1_0_num_1000"}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "5000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_5000"},{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_1"}],"opid":"3","extra":null},{"name":"SEQ_SCAN ","children":[],"opid":"4","extra":"polynomial_table_control_0"}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "10000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_10000"},{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_1"}],"opid":"3","extra":null},{"name":"SEQ_SCAN ","children":[],"opid":"4","extra":"polynomial_table_control_0"}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "50000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_50000"},{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_1"}],"opid":"3","extra":null},{"name":"SEQ_SCAN ","children":[],"opid":"4","extra":"polynomial_table_control_0"}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "100000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_100000"},{"name":"HASH_JOIN","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_1"}],"opid":"3","extra":null},{"name":"SEQ_SCAN ","children":[],"opid":"4","extra":"polynomial_table_control_0"}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "500000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_500000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"3","extra":"polynomial_table_control_1"}],"opid":"4","extra":null}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
        "1000000": '{"name":"PROJECTION","children":[{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"0","extra":"skew_1_0_num_1000000"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"1","extra":"polynomial_table_control_0"},{"name":"HASH_JOIN","children":[{"name":"SEQ_SCAN ","children":[],"opid":"2","extra":"polynomial_table_control_2"},{"name":"SEQ_SCAN ","children":[],"opid":"3","extra":"polynomial_table_control_1"}],"opid":"4","extra":null}],"opid":"5","extra":null}],"opid":"6","extra":null}],"opid":"7","extra":null}',
    },
}


# this basically goes over all the specs, and tries to figure out which
# SQLs are unique.
# Sigh.
def make_dump_files():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--out_dir", required=True)
    parsed = parser.parse_args()
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    spec_item = {
        num_join: dedup_res(join_result) for (num_join, join_result) in ALL_SPEC.items()
    }
    for num_join, plan_row_map in spec_item.items():
        num_join_dir = out_dir / num_join
        os.makedirs(num_join_dir, exist_ok=True)
        for plan, row_count in plan_row_map.items():
            file_name = "base" if len(row_count) > 1 else str(row_count[0])
            raw_plan_file = just_write("/tmp/raw_plan.json", plan)
            current_tmp_file = "/tmp/lineage_view.sql"
            traceprov_assert_safe_run(
                f"python3 {parsed.exe} {raw_plan_file} > {current_tmp_file}"
            )
            current_sql = just_read(current_tmp_file)
            seq_scan_map = extract_seq_scans(plan)
            seq_scan_map_serialized = "\n".join(
                [f"-- {key}: {value}" for (key, value) in seq_scan_map.items()]
            )
            current_sql = f"{current_sql} \n\n {seq_scan_map_serialized}"
            just_write(num_join_dir / f"{file_name}.sql", current_sql)


def extract_seq_scans(plan_str):
    content = json.loads(plan_str)

    def _extract_seq_scan(plan):
        plan_name: str = plan["name"]
        plan_name = plan_name.lower().strip()
        if plan_name == "seq_scan":
            return {plan["extra"]: plan["opid"]}
        joined = dict()
        for child in plan["children"]:
            joined = {**joined, **_extract_seq_scan(child)}
        return joined

    seq_scan_map = _extract_seq_scan(content)
    return seq_scan_map


# flip the res
def dedup_res(join_result):
    plan_row_map = dict()
    for num_row, plan in join_result.items():
        plan_row_map = {**plan_row_map, plan: [*(plan_row_map.get(plan, [])), num_row]}
    return plan_row_map


if __name__ == "__main__":
    make_dump_files()
