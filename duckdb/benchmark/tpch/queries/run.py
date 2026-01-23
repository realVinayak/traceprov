import argparse
from datetime import datetime
import json
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions


def get_in_extra(qdata, key):
    return qdata["traceprov"]["extras"][0][key]["captured"][0][0]


query_list = [1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 17, 18, 19, 20, 21, 22]
needs_disable = {11, 17, 18, 2, 20, 22}


def load_json(name: str):
    with open(name) as f:
        result = json.loads(f.read())
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--qnum", required=False)
    parsed = parser.parse_args()
    with open("spec_2.json") as f:
        spec = json.loads(f.read())["result"]["params_default"]
    reduced = {
        qnum: dict(
            capture=get_in_extra(qdata, "traceprov_capture_query"),
            derive=json.loads(get_in_extra(qdata, "traceprov_generic_derivation_spec"))[
                "elements"
            ],
            min_local_used=json.loads(
                get_in_extra(qdata, "traceprov_generic_derivation_spec")
            )["min_local_used"],
        )
        for (qnum, qdata) in spec.items()
    }

    root = Path("./root")
    all_results = dict()
    for query_num, query_data in reduced.items():
        int_query = int(query_num)
        if int_query not in query_list:
            continue
        if parsed.qnum is not None and parsed.qnum != str(query_num): continue
        path = root / Path(query_num)

        captured_sql = path / "capture.sql"
        base_sql = path / "base.sql"
        base_options = DuckDBDriverOptions(
            db=parsed.db,
            repeat=8,
            threads=1,
            i=base_sql.as_posix(),
            disable_col_opt=(int_query in needs_disable),
            time=(path / f"base_spec.json").as_posix(),
            min_layer_number=query_data["min_local_used"],
        )
        traceprov_assert_safe_run(f"{parsed.exe} {base_options.serialize()}")
        options = base_options._replace(
            i=captured_sql.as_posix(), time=(path / f"capture_spec.json").as_posix()
        )
        print(options.serialize())
        graph_path = path / "graph.bin"

        traceprov_assert_safe_run("rm -rf traceprov/*")
        #traceprov_assert_safe_run(f"cp {graph_path.as_posix()} traceprov/")
        traceprov_assert_safe_run(f"{parsed.exe} {options.serialize()}")

        options = options._replace(repeat=1)

        all_derive_options = dict()
        for element in query_data["derive"]:
            el_id = element["idx"]
            derive_query = path / f"infer_{el_id}.sql"
            with open(derive_query) as f:
                el_query = f.read()

            with open(path / f"infer_{el_id}_csv.sql", "w") as f:
                infer_out = f"infer_{el_id}_out.csv"
                csved = (
                    f"copy ({el_query}) to '{(path / infer_out).absolute().as_posix()}'"
                )
                f.write(csved)

            with open(path / f"infer_{el_id}_count.sql", "w") as f:
                infer_count = f"infer_{el_id}_count.csv"
                csved = f"copy (select count(*) from ({el_query}) f) to '{(path / infer_count).absolute().as_posix()}'"
                f.write(csved)
            dump_csv_query = path / f"infer_{el_id}_csv.sql"
            dump_count_query = path / f"infer_{el_id}_count.sql"
            derive_options = options._replace(
                extra=derive_query.as_posix(),
                time=(path / f"infer_spec_{el_id}.json").as_posix(),
            )
            traceprov_assert_safe_run(f"{parsed.exe} {derive_options.serialize()}")
            derive_csv_options = options._replace(
                extra=dump_csv_query.as_posix(), time=None
            )
            traceprov_assert_safe_run(f"{parsed.exe} {derive_csv_options.serialize()}")
            derive_count_options = options._replace(
                extra=dump_count_query.as_posix(), time=None
            )
            traceprov_assert_safe_run(
                f"{parsed.exe} {derive_count_options.serialize()}"
            )
            all_derive_options[el_id] = load_json(derive_options.time)

        all_results[query_num] = dict(
            base=load_json(base_options.time),
            forward=load_json(options.time),
            derive=all_derive_options,
        )
    current_timestamp = datetime.now()
    datetime_string = current_timestamp.strftime("%Y_%m_%d_%H_%M_%S")
    result_dir = Path(f"results/local_test_result_{datetime_string}/")
    os.makedirs(result_dir, exist_ok=True)
    with open(result_dir / "result.json", "w") as f:
        f.write(json.dumps(all_results))


if __name__ == "__main__":
    main()
