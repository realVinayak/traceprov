import os
from pathlib import Path

from traceprovpy.tools.run_with_timeout import RunWithTimeoutOptions


def provsql_get_log_size(args):
    run_time_options: RunWithTimeoutOptions = args["extra_pack"]
    conn = run_time_options.run_connection_strict()
    cursor = conn.cursor()
    cursor.execute("SELECT oid FROM pg_database WHERE datname = current_database();")
    db_oid = cursor.fetchall()[0][0]
    cursor.execute("show data_directory;")
    data_dir = cursor.fetchall()[0][0]
    core_dir = Path(data_dir) / "base" / str(db_oid)
    files = [f for f in Path(core_dir).iterdir() if f.is_file() and 'provsql' in f.name.lower()]
    file_sizes = {file.name: os.path.getsize(file) for file in files}
    print(file_sizes)
    return file_sizes
