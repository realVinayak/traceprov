import argparse
from typing import NamedTuple, Optional
import os

from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
from traceprovpy.tools.extract_gprom_simple import (
    GPROM_OPTIONS_MAPPING,
    GpromOptions,
    gprom_from_file,
    gprom_from_parsed,
)
from traceprovpy.tools.run_with_timeout import ConnectionParams


TABLE_TICKER = "%TABLE%"
GUESSED_ID_TICKER = "%GUESSED_ID%"


def open_template(subdir, qnum):
    file_name = f"templates/{subdir}/{qnum}.sql"
    with open(file_name) as f:
        template = f.read()
    return template


def cleanup(raw: str, guessed: str, table: str):
    cleaned = raw.replace(TABLE_TICKER, table)
    return cleaned.replace(GUESSED_ID_TICKER, guessed)


def main():
    parser = argparse.ArgumentParser(prog="microagg-query-gen")
    parser.add_argument("-qnum", "--qnum", required=True, type=str)
    parser.add_argument("-t", "--table_name", required=True, type=str)
    parser.add_argument(
        "--remove", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("-gnum", required=False, type=str, default="./")

    postgres_connection_from_cmd(parser)
    # GpromOptions.add_parse_options(parser)

    parsed, _ = parser.parse_known_args()
    query_dir = f"{parsed.gnum}/queries_{parsed.table_name}/{parsed.qnum}/"

    if parsed.remove:
        os.system(f"rm -rf {query_dir}/")
        assert 0 == os.system(f"mkdir -p {query_dir}")

    guessed_id_column = f'prov_{parsed.table_name.replace("_", "__")}_id'
    table_name = parsed.table_name

    template_base = cleanup(
        open_template(f"base/{parsed.gnum}/", parsed.qnum),
        guessed_id_column,
        table_name,
    )
    template_traceprov = cleanup(
        open_template(f"traceprov/{parsed.gnum}", parsed.qnum),
        guessed_id_column,
        table_name,
    )
    template_extract_gprom = cleanup(
        open_template(f"extract/{parsed.gnum}", parsed.qnum),
        guessed_id_column,
        table_name,
    )

    with open(f"{query_dir}/base.sql", "w") as f:
        f.write(template_base)

    with open(f"{query_dir}/traceprov.sql", "w") as f:
        f.write(template_traceprov)

    with open(f"{query_dir}/template_extract_gprom.sql", "w") as f:
        f.write(template_extract_gprom)

    # Now, need to run GProM on the extract queries.
    # We'd need to get two queries: (1) simple run (2) materialization
    # But, the second one is a bit tricky to get right. So, it is just (1)
    # The (2) is manaually added.

    absolute_dir = f"{os.getcwd()}/{query_dir}"
    absolute_input_path = f"{absolute_dir}/template_extract_gprom.sql"

    connection_params = ConnectionParams.make_from_parsed(parsed)
    for gprom_mode in GPROM_OPTIONS_MAPPING:
        gprom_sql = gprom_from_file(
            GPROM_OPTIONS_MAPPING[gprom_mode], connection_params, absolute_input_path
        )
        with open(f"{absolute_dir}/gprom_{gprom_mode}.sql", "w") as f:
            f.write(gprom_sql.strip())

        # absolute_mat_path = f"{absolute_dir}/gprom_{gprom_mode}.materialize.sql"
        with open(f"{absolute_dir}/gprom_{gprom_mode}.materialize.sql", "w") as f:
            f.write(gprom_sql.strip())


if __name__ == "__main__":
    main()
