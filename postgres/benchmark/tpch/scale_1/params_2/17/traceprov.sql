-- using 1755703766 as a seed to the RNG


SELECT *, mark_later(mapped_agg_second) FROM (
    select
        sum(l_extendedprice) / 7.0 as avg_yearly,
        traceprov_agg_key_parallel(1, p_partkey, l_orderkey, l_linenumber, mark_later(mapped_agg)) as mapped_agg_second
    FROM part, lineitem JOIN LATERAL (
            SELECT 
                0.2 * avg(l_quantity) as avg,
                traceprov_agg_key_parallel(4, l_orderkey, l_linenumber) as mapped_agg
                FROM lineitem
                WHERE l_partkey = p_partkey
        ) f
        ON l_quantity < f.avg
        WHERE p_partkey = l_partkey
        and p_brand = 'Brand#35'
        and p_container = 'SM PKG'
) g;

