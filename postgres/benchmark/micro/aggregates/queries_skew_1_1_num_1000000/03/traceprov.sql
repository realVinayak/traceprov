select mark_later(mapped_agg) FROM (
    select 
        sum(val), 
        z, 
        traceprov_agg_key_parallel(1, id) as mapped_agg 
    from skew_1_1_num_1000000
        group by z
) f;