def make_factor_replacer(factor_value: int):
    def _replacer(in_sql: str):
        assert "NUM" in in_sql
        query_str = in_sql.replace("NUM", str(1 << factor_value))
        return query_str

    return _replacer


def make_join_replacer(factor_value: int):
    def _replacer(in_sql: str):
        assert "NUM" in in_sql
        query_str = in_sql.replace("NUM", str(factor_value))
        return query_str

    return _replacer


def create_or_replace_table(query: str, table_name: str):
    replaced_query = query.replace(";", "")
    replaced_query = f"create or replace table {table_name} as ({replaced_query})"
    return replaced_query
