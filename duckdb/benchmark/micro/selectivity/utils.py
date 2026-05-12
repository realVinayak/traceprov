def make_replacer(
    num_rows, selectivity, is_random, selectivity_clause: str = ":selectivity"
):
    def replacer(in_sql: str):
        query_str = in_sql.replace("ROW_COUNT", str(num_rows))
        if selectivity:
            query_str = query_str.replace(selectivity_clause, str(selectivity))
        if not is_random:
            query_str = query_str.replace("_random", "")
        return query_str

    return replacer
