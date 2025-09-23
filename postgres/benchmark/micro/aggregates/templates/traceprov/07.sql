select mark_later(mapped_agg) FROM (
    select count(*) as c, z, traceprov_agg_key_parallel(1, d) as mapped_agg from %TABLE% group by z
) ORDER BY c desc limit 10;