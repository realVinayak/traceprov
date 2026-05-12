def make_replacer(num_rows, selectivity, is_random):
    def replacer(in_sql: str):
        query_str = in_sql.replace("ROW_COUNT", str(num_rows))
        if selectivity:
            query_str = query_str.replace(":selectivity", str(selectivity))
        if not is_random:
            query_str = query_str.replace("_random", "")
        return query_str

    return replacer


def get_filter_group(num_groups, selectivity, mode):
    multiplier = -1 if mode == "pre" else 1
    print(num_groups * selectivity, "num_gs")
    return int(selectivity * num_groups / 100) * multiplier
