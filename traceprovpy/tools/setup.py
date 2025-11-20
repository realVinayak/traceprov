# sets up traceprovpy.
from traceprovpy.tools.run_with_timeout import ConnectionParams
import os
from pathlib import Path


def traceprov_setup(
    suff: str, traceprov_postgres_root: str, connection_params: ConnectionParams
):
    assert suff is not None

    response = os.system(
        f"cd {traceprov_postgres_root} && ./build_and_install.sh {suff}"
    )
    if response != 0:
        raise Exception("Make failed!")
    traceprov_sql = Path(traceprov_postgres_root) / f"traceprov_{suff}.auto.sql"
    traceprov_infer_set = (
        Path(traceprov_postgres_root) / f"traceprov_return_infer_{suff}.auto.sql"
    )
    traceprov_ptr_type = Path(traceprov_postgres_root) / f"traceprov_"
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
    return dict(
        traceprov_path=traceprov_obj,
        traceprov_infer_set_path=traceprv_infer_set_obj,
        traceprov_rewriter_path=rewriter_obj,
    )
