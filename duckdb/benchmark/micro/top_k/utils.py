def make_replacer(num_rows, top_k):
    def replacer(in_sql: str):
        query_str = in_sql.replace("ROW_COUNT", str(num_rows))
        if top_k:
            query_str = query_str.replace(":top_k_limit", str(top_k))
        return query_str

    return replacer
