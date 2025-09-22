import argparse
from typing import NamedTuple, Optional
import os


class Options(NamedTuple):
    table_name: str
    qnum: str
    gprom_dir: Optional[str]
    db_name: str

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
    parser.add_argument("-db", "--db_name", required=True, type=str)
    parser.add_argument("-qnum", "--qnum", required=True, type=str)
    parser.add_argument("-gp_dir", "--gprom_dir", required=False, type=str)
    parser.add_argument("-t", "--table_name", required=False, type=str)

    parsed: Options = parser.parse_args()
    query_dir = f"queries_{parsed.table_name}/{parsed.qnum}/"
    os.system(f"rm -rf {query_dir}/")

    assert 0 == os.system(f"mkdir -p {query_dir}")

    guessed_id_column = f'prov_{parsed.table_name.replace("_", "__")}_id'
    table_name = parsed.table_name

    template_base = cleanup(
        open_template("base", parsed.qnum), guessed_id_column, table_name
    )
    template_traceprov = cleanup(
        open_template("traceprov", parsed.qnum), guessed_id_column, table_name
    )
    template_extract_gprom = cleanup(
        open_template("extract", parsed.qnum), guessed_id_column, table_name
    )

    with open(f"{query_dir}/base.sql", 'w') as f:
        f.write(template_base)

    with open(f"{query_dir}/traceprov.sql", 'w') as f:
        f.write(template_traceprov)

    with open(f"{query_dir}/template_extract_gprom.sql", 'w') as f:
        f.write(template_extract_gprom)

    if parsed.gprom_dir is None:
        return

    # Now, need to run GProM on the extract queries.
    # We'd need to get two queries: (1) simple run (2) materialization
    # But, the second one is a bit tricky to get right. So, it is just (1)
    # The (2) is manaually added.

    absolute_dir = f"{os.getcwd()}/{query_dir}"
    absolute_input_path = f"{absolute_dir}/template_extract_gprom.sql"

    out_paths = [("gprom_join", [""]), ("gprom_window", ["--window"])]

    gprom_commands = [
        f"python3 extract_gprom.py -db {parsed.db_name} -i {absolute_input_path} -o {absolute_dir}/{out_path}.sql {' '.join(out_options)}"
        for (out_path, out_options) in out_paths
    ]

    gprom_mat_commands = [
        f"python3 extract_gprom.py -db {parsed.db_name} -i {absolute_input_path} -o {absolute_dir}/{out_path}.materialize.sql {' '.join(out_options)}"
        for (out_path, out_options) in out_paths
    ]
    cmds = [f"cd {parsed.gprom_dir}", *gprom_commands, *gprom_mat_commands]
    all_cmds = ";".join(cmds)
    print('gprom: ', all_cmds)
    assert os.system(all_cmds) == 0

if __name__ == '__main__':
    main()
