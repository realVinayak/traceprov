def make_replacer(num_rows: int):
    def replacer(in_sql: str):
        query_str = in_sql.replace("ROW_COUNT", str(num_rows))
        return query_str

    return replacer


# ALL_QUERY_LIST = ["q01", "q02", "q03", "q04", "q06", "q07"]
ALL_QUERY_LIST = ["q03"]
