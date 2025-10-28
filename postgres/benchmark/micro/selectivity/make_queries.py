from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
import argparse
import os

from traceprovpy.tools.extract_gprom_simple import (
    GPROM_OPTIONS_MAPPING,
    GpromOptions,
    gprom_from_file,
)
from traceprovpy.tools.run_with_timeout import ConnectionParams


def replace_contents_and_mode(replace_num_by, file, mode):
    with open(f"./templates/predicate_{mode}/{file}.sql") as f:
        raw_base_sql = f.read()
        replaced_sql = raw_base_sql.replace("%DIR%", replace_num_by)
        data_table = f"data_table_{replace_num_by}".replace("_", "__")
        replaced_sql = replaced_sql.replace("%TABLE_ID%", f"prov_{data_table}_id")

    os.makedirs(f"./{replace_num_by}/predicate_{mode}/", exist_ok=True)
    created_file = f"./{replace_num_by}/predicate_{mode}/{file}.sql"
    with open(created_file, "w") as f:
        f.write(replaced_sql)
    return created_file


def main():
    parser = argparse.ArgumentParser(prog="selectivity-make-queries")
    parser.add_argument("-d", "--dir", required=True)
    parser.add_argument("-m", "--mode", type=str, required=True)

    postgres_connection_from_cmd(parser)
    parsed = parser.parse_args()

    if parsed.mode == "pre":
        mode = "pre"
    elif parsed.mode == "post":
        mode = "post"
    else:
        raise Exception(f"got invalid value of mode: {parsed.mode}")

    replace_contents_and_mode(parsed.dir, "base", mode)
    gprom_extract_file = replace_contents_and_mode(parsed.dir, "gprom_extract", mode)

    for gprom_mode in GPROM_OPTIONS_MAPPING:

        gprom_created_file = gprom_from_file(
            GPROM_OPTIONS_MAPPING[gprom_mode],
            ConnectionParams.make_from_parsed(parsed),
            gprom_extract_file,
        )

        assert gprom_extract_file is not None

        computed_out_file = f"./{parsed.dir}/predicate_{mode}/gprom_{gprom_mode}.sql"
        assert not os.path.exists(computed_out_file)
        with open(computed_out_file, "w") as f:
            f.write(gprom_created_file.strip())


if __name__ == "__main__":
    main()
