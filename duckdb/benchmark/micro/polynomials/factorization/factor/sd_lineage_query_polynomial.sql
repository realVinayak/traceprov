with lineage_query_result(out_id, table_id, table_row_id) as (
    select
        *
    from
        lineage_query(0, 100, 0 :: UINTEGER)
)
select
    string_agg(table_prov, ' ⊗ ') as polynomial
from
    (
        select
            '(' || string_agg(table_row_id :: text, ' ⊕ ') || ')' as table_prov,
            table_id
        from
            lineage_query_result
        group by
            table_id
    )