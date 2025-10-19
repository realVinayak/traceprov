SELECT mark_later(mapped_agg) FROM (
    select 
        min(min_value) as min_over_group, 
        group_number,
        traceprov_agg_key_parallel(1, id) as mapped_agg
    from data_table_%DIR%
    group by group_number 
    having min(min_value) <= :selectivity
) F;