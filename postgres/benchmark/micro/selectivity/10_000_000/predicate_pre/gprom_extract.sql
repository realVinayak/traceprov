SELECT prov_data__table__10__000__000_id FROM (
PROVENANCE OF (
    select 
        min(min_value) as min_over_group, 
        group_number 
    from data_table_10_000_000
    USE PROVENANCE (id)
    where negative_group_number >= :selectivity
    group by group_number
) ) F;