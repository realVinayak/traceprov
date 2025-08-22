-- using 1755693634 as a seed to the RNG


-- select
--     sum(l_extendedprice) / 7.0 as avg_yearly
-- from
--     lineitem,
--     part
-- where
--     p_partkey = l_partkey
--     and p_brand = 'Brand#32'
--     and p_container = 'WRAP JAR'
--     and l_quantity < (
--         select
--             0.2 * avg(l_quantity)
--         from
--             lineitem
--         where
--             l_partkey = p_partkey
--     );

SELECT *, mark_later(mapped_agg_second) FROM (
    SELECT 
        SUM(l_extendedprice) / 7.0 AS avg_yearly, 
        agg_map_parallel_second(p_partkey, l_orderkey, l_linenumber, 0) as mapped_agg_second,
        sum(later)
    FROM 
        (SELECT 
            *,
            mark_later(mapped_agg) as later
        FROM   part, lineitem
            JOIN LATERAL (
                    SELECT 
                        0.2 * Avg(l_quantity) as avg, 
                        agg_map_parallel(l_orderkey, l_linenumber, 0, 0) as mapped_agg
                    FROM   lineitem
                    WHERE  l_partkey = p_partkey
            ) f
            ON l_quantity < f.avg
        WHERE  p_partkey = l_partkey
            AND p_brand = 'Brand#32'
            AND p_container = 'WRAP JAR') h
) g;
