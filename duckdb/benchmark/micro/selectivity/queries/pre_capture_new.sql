SELECT *,
    traceprov_log_entry_1 (2, mapped_agg) 
FROM (
    select 
        min(min_value) as min_over_group, 
        group_number,
        traceprov_agg_key_parallel_offset_1(1, rowid::bigint) as mapped_agg
    from data_table_ROW_COUNT_random
    where negative_group_number >= :selectivity
    group by group_number 
    order by min_over_group
) F;