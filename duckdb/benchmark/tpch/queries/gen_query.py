import json
import os
from pathlib import Path
import sys


def get_in_extra(qdata, key):
    return qdata["traceprov"]["extras"][0][key]["captured"][0][0]


def main():
    assert sys.argv[1]
    with open(sys.argv[1]) as f:
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

    print(reduced["2"]["capture"])
    print(reduced["2"]["derive"])

    os.system("rm -rf root/")

    root = Path("./root")
    for query_num, query_data in reduced.items():
        path = root / Path(query_num)
        os.makedirs(path, exist_ok=True)

        with open(path / "capture.sql", "w") as f:
            f.write(query_data["capture"])

        for element in query_data["derive"]:
            el_id = element["idx"]
            el_query = element["sql"]
            with open(path / f"infer_{el_id}.sql", "w") as f:
                f.write(el_query)

            with open(path / f"infer_{el_id}_csv.sql", "w") as f:
                infer_out = f"infer_{el_id}_out.csv"
                csved = (
                    f"copy ({el_query}) to '{(path / infer_out).absolute().as_posix()}'"
                )
                f.write(csved)

            with open(path / f"infer_{el_id}_count.sql", "w") as f:
                infer_count = f"infer_{el_id}_count.csv"
                csved = f"copy (select count(*) from ({el_query}) f) to '{(path / infer_count).absolute().as_posix()}.csv'"
                f.write(csved)

        with open(path / "graph.bin", "wb") as f:
            min_local_used: int = int(query_data["min_local_used"])
            binary_data = min_local_used.to_bytes(4, byteorder=sys.byteorder)
            f.write(binary_data)


if __name__ == "__main__":
    main()
