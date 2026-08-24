with lineage_query_result(out_id, table_id, table_row_id) as (
    select
        *
    from
        lineage_query(0, 100, OUT_ID :: UINTEGER)
)
select
    string_agg(lineage_query_result.table_row_id :: text, ' ⊗ ') as polynomial
from
    lineage_query_result