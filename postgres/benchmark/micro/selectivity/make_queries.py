from traceprovpy.tools.connection_utils import postgres_connection_from_cmd
import argparse
import os

base_gprom_executable = "gprom -passwd postgres -host 127.0.0.1 -port 5432 -user postgres -backend postgres -db playground"
gprom_modes = {
    "join": [],
    "window": ["-prov_instrument_agg_window -prov_use_composable"],
    "join_heuristics": ["-heuristic_opt TRUE"],
    "window_heuristics": [
        "-prov_instrument_agg_window -prov_use_composable -heuristic_opt TRUE"
    ],
}


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

    base_user_options = dict(
        user=parsed.user,
        passwd=parsed.password,
        host=parsed.host,
        port=parsed.port,
        db=parsed.db,
        Pexecutor="sql",
        Loperator_verbose="TRUE",
    )

    flattened = " ".join(f"-{key} {value}" for key, value in base_user_options.items())
    execute_cmd_raw = f"gprom -backend postgres {flattened}"

    replace_contents_and_mode(parsed.dir, "base", mode)
    gprom_extract_file = replace_contents_and_mode(parsed.dir, "gprom_extract", mode)

    for gprom_mode in gprom_modes:
        gprom_temp_file = "./gprom_created.tmp"
        os.system(f"rm -f {gprom_temp_file}")
        execute_cmd = [execute_cmd_raw, *gprom_modes[gprom_mode]]
        in_file = ["-queryFile", gprom_extract_file]
        out_file = ["-Poutfile", gprom_temp_file]
        gprom_executable = [*execute_cmd, *in_file, *out_file]
        # print(gprom_executable)
        gprom_command = " ".join(gprom_executable)
        os.system(gprom_command)
        try:
            with open(gprom_temp_file) as f:
                gprom_created_file = f.read()
        except FileNotFoundError:
            raise Exception("Didn't created the gprom file!")
        computed_out_file = f"./{parsed.dir}/predicate_{mode}/gprom_{gprom_mode}.sql"
        assert not os.path.exists(computed_out_file)
        with open(computed_out_file, "w") as f:
            f.write(gprom_created_file.strip())


if __name__ == "__main__":
    main()
