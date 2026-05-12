import argparse
from pathlib import Path
import sys

from traceprovpy.tools.connection_utils import duckdb_connection_from_cmd
from traceprovpy.tools.extract_gprom_simple import (
    GPROM_OPTIONS_MAPPING,
    GpromOptions,
    gprom_from_file,
)
from traceprovpy.tools.file_utils import just_read, just_write
from traceprovpy.tools.run_with_timeout import ConnectionParams
from utils import ALL_QUERY_LIST, make_replacer


def main():
    parser = argparse.ArgumentParser(prog="aggregates-gprom")
    parser.add_argument("-s", "--source", type=str, required=True)
    GpromOptions.add_parse_options(parser)
    curr_args = " ".join(sys.argv)
    if "--backend duckdb" in curr_args:
        duckdb_connection_from_cmd(parser)
    else:
        assert False, "Invalid backend"
    parsed, _ = parser.parse_known_args()
    connection_params = ConnectionParams.make_from_parsed(
        parsed, backend=parsed.backend
    )
    replacer = make_replacer("1000000")
    for query in ALL_QUERY_LIST:
        absolute_input_path = Path(parsed.source) / query / f"gprom_extract.sql"
        input_query = just_read(absolute_input_path)
        out_path = just_write(
            Path(parsed.source) / query / f"gprom_extract.tmp.sql",
            replacer(input_query),
        )
        for gprom_mode in GPROM_OPTIONS_MAPPING:
            options = GPROM_OPTIONS_MAPPING[gprom_mode]
            gprom_sql = gprom_from_file(options, connection_params, out_path)
            gprom_out_path = Path(parsed.source) / query / f"{gprom_mode}.sql"
            just_write(gprom_out_path, gprom_sql)


if __name__ == "__main__":
    main()
