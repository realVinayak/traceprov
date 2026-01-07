# Simply copies over the traceprov files.

import os
from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.run_with_timeout import ConnectionParams
import getpass


def make_copy(connection_params: ConnectionParams, destination_dir: str):
    # eh, works well enough.
    original_user = getpass.getuser()
    connection = connection_params.make_simple_connection()
    cursor = connection.cursor()
    cursor.execute("SHOW data_directory;")
    result = cursor.fetchall()
    data_dir = result[0][0]
    cursor.close()
    connection.close()

    assert os.system(f"mkdir -p {destination_dir}") == 0

    copy_csv_func = f"""
    #!/bin/bash
    sudo -s <<EOF
    cp {data_dir}/traceprov/*.csv {destination_dir}
    EOF
    """

    traceprov_assert_safe_run(copy_csv_func)
    traceprov_assert_safe_run(f"sudo chown -R {original_user}:{original_user} {destination_dir}")