select *, mark_later(mapped_agg), traceprov_infer_poly(1, mapped_agg, 1, 0, 0) FROM  (
    select 
        min(min_value) as min_over_group, 
        group_number,
        traceprov_agg_key_parallel(1, id) as mapped_agg
    from data_table_1_000_000
    where negative_group_number >= :selectivity
    group by group_number
) F;
