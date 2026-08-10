import os
from pathlib import Path
from time import sleep

from traceprovpy.tools.file_utils import (
    get_tmp_file,
    just_read,
    just_write,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.run_with_timeout import (
    MakeKeySelection,
    RunWithTimeoutOptions,
    run_with_timeout,
)


def get_db_path(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT oid FROM pg_database WHERE datname = current_database();")
    db_oid = cursor.fetchall()[0][0]
    cursor.execute("show data_directory;")
    data_dir = cursor.fetchall()[0][0]
    core_dir = Path(data_dir) / "base" / str(db_oid)
    cursor.close()
    return core_dir


def has_general_write_access(conn):
    core_dir = get_db_path(conn)
    files = [
        f
        for f in Path(core_dir).iterdir()
        if f.is_file() and "provsql" in f.name.lower()
    ]
    has_write_access = os.access(files[0], os.W_OK)
    return has_write_access


def _copy_provsql_files(conn, reverse=False):
    core_dir = get_db_path(conn)
    files = [
        f
        for f in Path(core_dir).iterdir()
        if f.is_file() and "provsql" in f.name.lower()
    ]
    has_write_access = os.access(files[0], os.W_OK)
    run_cmd = (
        traceprov_assert_safe_run
        if has_write_access
        else (lambda _cmd: traceprov_assert_safe_run(f'sudo su - postgres -c "{_cmd}"'))
    )
    tmp_dir = f"{core_dir.absolute().as_posix()}/tmp/"
    run_cmd(f"mkdir -p {tmp_dir}")
    for file in files:
        if reverse:
            run_cmd(f"cp {tmp_dir}/{file.name} {core_dir.absolute().as_posix()}")
        else:
            run_cmd(f"cp {file.absolute().as_posix()} {tmp_dir}/")


def backup_provsql_files(args):
    run_time_options: RunWithTimeoutOptions = args["extra_pack"]
    conn = run_time_options.connection_params.make_connection()
    _copy_provsql_files(conn, reverse=False)
    conn.close()


def restore_provsql_files(args):
    run_time_options: RunWithTimeoutOptions = args["extra_pack"]
    conn = run_time_options.run_connection_strict()
    _copy_provsql_files(conn, reverse=True)


def restart_mmap_writer(args):
    tmp_file = Path(get_tmp_file()) / "process_list.txt"
    traceprov_assert_safe_run(
        f"ps -eo pid,command | grep ProvSQL > {tmp_file.as_posix()}"
    )
    contents = just_read(tmp_file)
    for line in contents.splitlines():
        if "ProvSQL MMap Worker" in line:
            break
    else:
        raise Exception("expected to find the mmap worker!")
    line = line.strip()
    worker_pid = line.split(" ")[0]
    traceprov_assert_safe_run(f"sudo kill {worker_pid}")
    # worker will restart after 2s
    sleep(2)
    # Since ProvSQL does this lazily, this is actually fine.
    # that is, restoring the files like this is legal. the worker will have started,
    # but won't touch the contents till the first IPC (which guarantees to have been done on a "valid" state anyways)
    restore_provsql_files(args)
    return None


PROV_TOKEN = "_PROV_"


def make_provsql_backtrace_offset(_, query):
    current_query = query

    def _provsql_backtrace_offset(args):
        # print("ARGS: ", args)
        global_is_last = args["is_global_last"]
        if not global_is_last:
            return dict(type="early")
        run_time_options: RunWithTimeoutOptions = args["extra_pack"]
        total_repeat = (
            run_time_options.params.repeat + run_time_options.params.throwaway
        )
        capture_result = args["extra_results"]["backtrace_capture"][0]
        if capture_result is None:
            return dict(timeout=True)
        captured_rows = capture_result["captured"]
        all_results = []
        if current_query is None:
            new_current_query = just_read(run_time_options.file_path)
            assert (
                new_current_query is not None
            ), f"expected {run_time_options.file_path} to exist!"
        assert PROV_TOKEN in new_current_query
        for output_row_id, output_row in enumerate(captured_rows):
            provsql = output_row["provsql"]
            filtered_extra_pack = run_time_options._replace(
                capture_output=False, strict_run=False
            )
            new_query = new_current_query.replace(PROV_TOKEN, provsql)
            query_path = just_write("/tmp/prov_query_path.sql", new_query)
            filtered_extra_pack = filtered_extra_pack._replace(
                file_path=query_path,
            )
            current_results = []
            for extra_iter in range(total_repeat):
                print("EXTRA ITER: ", extra_iter)
                provsql_result = run_with_timeout(filtered_extra_pack)
                current_results.append(provsql_result)
            tuid_result_pack = dict(
                row_id=output_row_id, provsql=provsql, results=current_results
            )
            all_results.append(tuid_result_pack)
        return dict(backtrace_results=all_results)

    return _provsql_backtrace_offset


def provsql_get_log_size(args):
    run_time_options: RunWithTimeoutOptions = args["extra_pack"]
    conn = run_time_options.run_connection_strict()
    core_dir = get_db_path(conn)
    files = [
        f
        for f in Path(core_dir).iterdir()
        if f.is_file() and "provsql" in f.name.lower()
    ]
    file_sizes = {file.name: os.path.getsize(file) for file in files}
    print(file_sizes)
    return file_sizes
