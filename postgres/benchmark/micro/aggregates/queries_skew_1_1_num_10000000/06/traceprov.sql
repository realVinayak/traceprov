select mark_later(mapped_agg) FROM (
    select count(*) as c, z, traceprov_agg_key_parallel(1, d) as mapped_agg from skew_1_1_num_10000000 group by z
) ORDER BY c limit 10;