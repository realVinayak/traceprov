# sets up traceprovpy.
from traceprovpy.tools.run_with_timeout import ConnectionParams, SmokedDuckOptions
import os
from pathlib import Path


def traceprov_setup(
    suff: str,
    traceprov_postgres_root: str,
    connection_params: ConnectionParams,
    # the smokedduck shared library.
    sd_lib_path: str = "",
    sd_include_path: str = "",
):
    assert suff is not None

    skip_build = int(os.getenv("tp_skip_build", "0"))
    if not skip_build:
        build_and_install = f"./build_and_install.sh {suff}"
        if sd_lib_path:
            assert sd_include_path
            build_and_install = f"{build_and_install} {sd_lib_path} {sd_include_path}"
        response = os.system(f"cd {traceprov_postgres_root} && {build_and_install}")
        if response != 0:
            raise Exception("Make failed!")
    else:
        print("skipping tp build from scratch")
    traceprov_sql = Path(traceprov_postgres_root) / f"traceprov_{suff}.auto.sql"
    traceprov_infer_set = (
        Path(traceprov_postgres_root) / f"traceprov_return_infer_{suff}.auto.sql"
    )
    assert traceprov_sql.exists(), f"{traceprov_sql.as_posix()} should exist!"
    assert (
        traceprov_infer_set.exists()
    ), f"{traceprov_infer_set.as_posix()} should exist!"

    assert (
        os.system(
            f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_sql.as_posix()} -v ON_ERROR_STOP=1"
        )
        == 0
    )

    assert (
        os.system(
            f"PGPASSWORD={connection_params.password} psql {connection_params.get_flat()} -f {traceprov_infer_set.as_posix()} -v ON_ERROR_STOP=1"
        )
        == 0
    )

    traceprov_obj = f"libtraceprov{suff}"
    traceprv_infer_set_obj = f"libtraceprov_infer{suff}"
    rewriter_obj = f"librewriter{suff}"

    assert (
        os.system(
            f"echo \"load '{rewriter_obj}'\" | PGPASSWORD={connection_params.password} psql {connection_params.get_flat()}"
        )
        == 0
    )

    if sd_lib_path:
        sd_executable_path = (
            Path(traceprov_postgres_root) / f"bld/bin/run_smokedduck_{suff}"
        ).resolve()
        assert sd_executable_path.exists(), "smokedduck path should exist!"
        sd_options = SmokedDuckOptions(driver_executable=sd_executable_path.as_posix())
    else:
        sd_options = None
    return dict(
        traceprov_path=traceprov_obj,
        traceprov_infer_set_path=traceprv_infer_set_obj,
        traceprov_rewriter_path=rewriter_obj,
        sd_options=sd_options,
    )


# add the special traceprov ticker.
def traceprov_make_query(query: str):
    return f"/*(traceprov)*/ {query}"


def traceprov_reinit_state(params: ConnectionParams):
    flat_options = params.get_flat()
    assert (
        os.system(
            f'echo "select reinit_state();" | PGPASSWORD={params.password} psql {flat_options}'
        )
        == 0
    )
