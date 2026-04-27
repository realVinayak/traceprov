# Simply copies over the traceprov files.

import os
from traceprovpy.tools.file_utils import traceprov_assert_safe_run
from traceprovpy.tools.run_with_timeout import ConnectionParams
import getpass


def copy_csv_func(data_dir, destination_dir, is_reverse: bool = False):
    copy_args = [f"{data_dir}/traceprov/graph.bin", destination_dir]
    if is_reverse:
        copy_args.reverse()
    copy_args_dir = " ".join(copy_args)
    return f"""
#!/bin/bash
sudo -s <<"EOF"
cp {copy_args_dir}
EOF
"""


def make_copy(
    connection_params: ConnectionParams, destination_dir: str, is_reverse: bool = False
):
    # eh, works well enough.
    original_user = getpass.getuser()
    connection = connection_params.make_connection()
    cursor = connection.cursor()
    cursor.execute("SHOW data_directory;")
    result = cursor.fetchall()
    data_dir = result[0][0]
    cursor.close()
    connection.close()

    if not is_reverse:
        traceprov_assert_safe_run(f"mkdir -p {destination_dir}")

    with open("/tmp/prepare_for_duckdb.sh", "w") as f:
        f.write(copy_csv_func(data_dir, destination_dir, is_reverse).strip())

    traceprov_assert_safe_run("chmod +x /tmp/prepare_for_duckdb.sh")
    traceprov_assert_safe_run("/tmp/prepare_for_duckdb.sh")
    if is_reverse:
        traceprov_assert_safe_run(
            f"sudo chown -R postgres:postgres {data_dir}/traceprov"
        )
