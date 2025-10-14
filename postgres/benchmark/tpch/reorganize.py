# A simple python script (temporary) that moves the files up.

import argparse
import os
import re

QUERY_NUM_EXTRACT_RE = r"(\d+)\.[^nab]|(\d+\.no_limit)|(\d+\.a)|(\d+\.b)"


def main():
    parser = argparse.ArgumentParser("reorganize")
    parser.add_argument("-root", required=True)
    parser.add_argument("-new_root", required=True)
    parsed = parser.parse_args()

    root = parsed.root
    new_root = parsed.new_root

    os.makedirs(new_root, exist_ok=False)

    for root, param_dirs, leaf_files in os.walk(root):
        if len(param_dirs) == 0:
            # All these paths are the leafs (where our queries live)
            param_dir = root.split("/")[1]
            assert "." not in param_dir
            os.makedirs(f"{new_root}/{param_dir}", exist_ok=True)
            terminal_dir = root.split("/")[-1]
            if terminal_dir == "validate":
                terminal_dir = "validate_template"

            if "gprom" in root:
                continue
            for leaf_file in leaf_files:
                query_num_match = re.match(QUERY_NUM_EXTRACT_RE, leaf_file)
                assert query_num_match is not None
                query_num = (
                    query_num_match.group(1)
                    or query_num_match.group(2)
                    or query_num_match.group(3)
                    or query_num_match.group(4)
                )
                os.makedirs(f"{new_root}/{param_dir}/{query_num}", exist_ok=True)
                assert not os.path.exists(
                    f"{new_root}/{param_dir}/{query_num}/{terminal_dir}.sql"
                )
                print(root, query_num, leaf_file)
                os.system(
                    f"cp {root}/{leaf_file} {new_root}/{param_dir}/{query_num}/{terminal_dir}.sql"
                )


if __name__ == "__main__":
    main()
