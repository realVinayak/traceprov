SELECT prov_data__table__5__000__000_id FROM (
PROVENANCE OF (
    select 
        min(min_value) as min_over_group, 
        group_number 
    from data_table_5_000_000
    USE PROVENANCE (id)
    group by group_number 
    having min(min_value) <= :selectivity
) ) F;