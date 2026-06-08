from traceprovpy.tools.run_duckdb_generic import set_extra_traceprov_options


def set_extra(parsed, table_count, extra_options: list = None):
    if extra_options is None:
        extra_options = []
    added_extra_options = {
        "custom_graph_type": 2,
        "log_chain_table_count": table_count,
        "min_layer_number": 3,
    }
    flat = [f"--{key} {value}" for (key, value) in added_extra_options.items()]
    # flat.append("--sd_join_mode")
    for _extra in extra_options:
        if isinstance(_extra, str):
            key = _extra
            value = None
        else:
            assert isinstance(_extra, tuple)
            key = _extra[0]
            value = str(_extra[1])
        flat.append(f"--{key}")
        if value is not None:
            flat.append(str(value))
    combined = " ".join(flat)
    set_extra_traceprov_options(parsed, combined)
