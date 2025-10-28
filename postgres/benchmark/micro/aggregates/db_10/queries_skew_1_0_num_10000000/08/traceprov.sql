select mark_later(mapped_agg) FROM (
    select count(*) as c, z, traceprov_agg_key_parallel(1, id) as mapped_agg from skew_1_0_num_10000000 group by z
) F ORDER BY c limit 1;