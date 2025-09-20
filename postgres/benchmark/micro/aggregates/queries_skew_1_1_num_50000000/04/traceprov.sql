select *, mark_later(mapped_agg) FROM (
    select 
        avg(val), 
        z, 
        traceprov_agg_key_parallel(1, id) as mapped_agg 
    from skew_1_1_num_50000000
        group by z
        having avg(val) > 50
) f;