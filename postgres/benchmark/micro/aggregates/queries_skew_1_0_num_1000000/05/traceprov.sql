select mark_later(mapped_agg_later) FROM (
    select 
        min_value, count(f.z), traceprov_agg_from_ptr(1, 5, mapped_agg) as mapped_agg_later
        from (
                select min(val) as min_value, 
                    z,
                    traceprov_agg_key_parallel(1, id) as mapped_agg
                    from skew_1_0_num_1000000 
                    group by z
            ) f 
        group by min_value 
    having (count(f.z)) > 900
) F;
