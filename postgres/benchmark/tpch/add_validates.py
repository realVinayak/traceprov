# A simple python script (temporary) that adds validate scripts.

import argparse
import os
import json
import uuid

simple_uuid = lambda: str(uuid.uuid4()).split("-")[0]


def remove_keys(in_dict: dict, to_remove: list[str]):
    return {key: value for (key, value) in in_dict.items() if key not in to_remove}


def make_layered(config_file):
    layers = []
    initial_layer = config_file.get("layer_number", 1)
    reference = config_file.get("reference", 0)
    subq_layer = config_file.get("subq_layer", None)

    # For the tpc-h, to parse out the config properly, this propery needs to hold true/
    # traceprov can, still, handle it.
    assert reference is not None or subq_layer is not None

    initial_config = dict(
        layer_number=initial_layer,
        pk_order=config_file["pk_order"],
        pk_inserts=[
            remove_keys(insert, ["is_sub"])
            for insert in config_file["inserts"]
            if not insert.get("is_sub", False)
        ],
        reference_layer=reference,
        subq_layer=0,
    )
    layers.append(initial_config)

    if subq_layer:
        subq_config = dict(
            layer_number=0,
            pk_order=config_file["subq_pk_order"],
            pk_inserts=[
                remove_keys(insert, ["is_sub"])
                for insert in config_file["inserts"]
                if insert.get("is_sub", False)
            ],
            reference_layer=0,
            subq_layer=subq_layer,
        )
        layers.append(subq_config)

    return layers


def move_validates():
    for root, dirs, config_files in os.walk("./validate_configs"):
        assert len(dirs) == 0
        for config_file in config_files:
            config_file_path = f"{root}/{config_file}"
            with open(config_file_path) as f:
                raw_config = json.loads(f.read())
            layered = make_layered(raw_config)
            new_config_file = f"./validate_configs_layered/{config_file}"
            with open(new_config_file, "w") as f:
                f.write(json.dumps(layered, indent=4))


def main():
    parser = argparse.ArgumentParser("add-validates")
    parser.add_argument("-root", required=True)
    parsed = parser.parse_args()

    queries_root = parsed.root

    not_found = set()

    for root, param_dirs, leaf_files in os.walk(queries_root):
        if len(param_dirs) != 0:
            continue

        query_num = root.split("/")[-1]

        try:
            with open(f"validate_configs/{query_num}.config.json") as f:
                f.read()
        except FileNotFoundError:
            not_found.add(query_num)
            continue
        # We're at the leafs.
        print(root, leaf_files, query_num)

    print("NOT FOUND: ", not_found)


def add_validates():

    parser = argparse.ArgumentParser("add-validates")
    parser.add_argument("-tf", required=True)
    parsed = parser.parse_args()

    with open(parsed.tf) as f:
        function_template_file = f.read()

    for root, dirs, layered_config_files in os.walk("./validate_configs_layered"):
        assert len(dirs) == 0

        for layered_config_file in layered_config_files:
            print(layered_config_file)
            with open(f"{root}/{layered_config_file}") as f:
                layered_config = json.loads(f.read())

            query_idx = layered_config_file.replace(".config.json", "")
            os.makedirs(f"./templates/validate_layered/{query_idx}/", exist_ok=False)

            out_path = f"./templates/validate_layered/{query_idx}/"

            for layer_id, layer in enumerate(layered_config):
                # every layer gets its own function and table (for now, just for validation)
                # we can, theoretically, share functions. But, this is temporary for validation
                # So, it is not something we _need_ to optimize.
                pk_logged = [*layer["pk_order"]]
                if len(pk_logged) == 1:
                    pk_logged = [*pk_logged, "sample_column"]
                outs = ",".join([f"OUT {key} INTEGER" for key in pk_logged])
                function_suffix = simple_uuid()
                func_sql = function_template_file.replace("%A%", function_suffix)
                func_sql = func_sql.replace("%OUT%", outs)
                function_name = f"traceprov_infer_{function_suffix}"
                create_func_sql = func_sql
                drop_table_sql = f"DROP TABLE IF EXISTS layer_{layer_id};"
                create_temp_table_sql = f"CREATE TEMP TABLE layer_{layer_id} AS SELECT * FROM {function_name}({layer['layer_number']}, {layer['reference_layer']}, {layer['subq_layer']});"
                drop_func_sql = f"drop function if exists {function_name};"

                sql_pack = dict(
                    drop_table=drop_table_sql,
                    drop_function=drop_func_sql,
                    create_function=create_func_sql,
                    create_temp_table=create_temp_table_sql,
                )

                for key, value in sql_pack.items():
                    out_file_name = f"{out_path}/layer_{layer_id}_{key}.sql"
                    with open(out_file_name, "x") as f:
                        f.write(value)


if __name__ == "__main__":
    # main()
    # move_validates()
    add_validates()
    ...
