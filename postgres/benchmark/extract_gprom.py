import argparse
import os
import json
import datetime


def get_config(file):
    with open(file) as f:
        config = json.loads(f.read())
    return config


def main():
    parser = argparse.ArgumentParser(prog="extract-gprom")
    parser.add_argument("-i", "--input", required=True, type=str)
    parser.add_argument("-o", "--output", required=False, type=str)
    parser.add_argument(
        "--window", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--lateral_rew", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "-cfg", "--config", required=False, type=str, default=".config.json"
    )
    parser.add_argument("-db", "--database", required=True, type=str)
    parser.add_argument("-v", "--verbose", required=False, type=int)
    parser.add_argument(
        "-heu", "--heuristics", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("-dir", "--out_dir", required=False)

    parsed = parser.parse_args()

    print("extracting: ", parsed)
    config = get_config(parsed.config)
    config = {**config, "db": parsed.database}

    window_args = ["-prov_instrument_agg_window", "-prov_use_composable"]
    cmd_options = ["-Pexecutor sql"]

    if parsed.window:
        cmd_options = [*window_args, *cmd_options]

    if parsed.lateral_rew:
        cmd_options.extend(["-lateral_rewrite"])

    if parsed.heuristics:
        cmd_options.extend(["-heuristic_opt TRUE"])

    if parsed.verbose:
        cmd_options.append("-Loperator_verbose TRUE")
    gprom_options = " ".join([f"-{key} {value}" for key, value in config.items()])
    gprom_executable = f"gprom {gprom_options} {' '.join(cmd_options)}"

    with open(parsed.input) as pi:
        cleaned = pi.read().replace("\n", " ").replace("\t", "    ")

    with open("/tmp/gprom_base.sql", "w") as f:
        f.write(cleaned)

    print(gprom_executable)
    execute_cmd = (
        f'echo "\i /tmp/gprom_base.sql" | {gprom_executable} | tee /tmp/gprom.out'
    )

    os.system(execute_cmd)

    if parsed.output is None:
        return

    with open("/tmp/gprom.out") as f:
        gprm_out = f.read()

    BEGIN = "/tmp/gprom_base.sql"
    END = ";"

    begin_index = gprm_out.index(BEGIN) + len(BEGIN)
    end_index = gprm_out.index(END)

    assert begin_index < end_index

    time = datetime.datetime.now()

    sql_str = (
        f"--- SQL OUT --- ON {time.replace(microsecond=0).isoformat()} \n"
        + gprm_out[begin_index : end_index + 1]
    )

    if parsed.verbose:
        print(sql_str)

    if parsed.output == "INF":

        parse_in_cleaned = parsed.input.split("/")[-1]

        if not parsed.heuristics:
            outfile = parse_in_cleaned
        else:
            outfile = parse_in_cleaned.replace(".sql", ".heuristics.sql")
        outfile = outfile.replace(".extract.", ".extracted.")
        assert outfile != parsed.input

        if parsed.out_dir:
            outfile = f"{parsed.out_dir}/{outfile}"
    else:
        outfile = parsed.output

    with open(outfile, "w") as po:
        po.write(sql_str)


if __name__ == "__main__":
    main()
