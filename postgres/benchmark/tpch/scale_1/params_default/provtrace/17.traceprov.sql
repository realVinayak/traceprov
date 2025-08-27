-- -- using default substitutions
SELECT *, mark_later(mapped_agg_second) FROM (
    SELECT 
        SUM(l_extendedprice) / 7.0 AS avg_yearly, 
        agg_map_parallel_second(p_partkey, l_orderkey, l_linenumber, mark_later(mapped_agg)) as mapped_agg_second
    FROM   part, lineitem
        JOIN LATERAL (
                SELECT 0.2 * Avg(l_quantity) as avg, agg_map_parallel(l_orderkey, l_linenumber, 0, 0) as mapped_agg
                FROM   lineitem
                WHERE  l_partkey = p_partkey
        ) f
        ON l_quantity < f.avg
    WHERE  p_partkey = l_partkey
        AND p_brand = 'Brand#23'
        AND p_container = 'MED BOX'
) g;

-- -- using default substitutions
-- SELECT *, mark_later(mapped_agg_second) FROM (
--     SELECT 
--         SUM(l_extendedprice) / 7.0 AS avg_yearly, 
--         agg_map_parallel_second(p_partkey, l_orderkey, l_linenumber, 0) as mapped_agg_second
--     FROM   part, lineitem
--         JOIN LATERAL (
--                 SELECT 0.2 * Avg(l_quantity) as avg, agg_map_parallel(l_orderkey, l_linenumber, 0, 0) as mapped_agg
--                 FROM   lineitem
--                 WHERE  l_partkey = p_partkey
--         ) f
--         ON l_quantity < f.avg
--     WHERE  p_partkey = l_partkey
--         AND p_brand = 'Brand#23'
--         AND p_container = 'MED BOX'
-- ) g;

-- -- using default substitutions
-- SELECT *, mark_later(mapped_agg_second) FROM (
--     SELECT 
--         SUM(l_extendedprice) / 7.0 AS avg_yearly, 
--         agg_map_parallel_second(p_partkey, l_orderkey, l_linenumber, 0) as mapped_agg_second,
--         sum(later)
--     FROM 
--         (SELECT 
--             *,
--             mark_later(mapped_agg) as later
--         FROM   part, lineitem
--             JOIN LATERAL (
--                     SELECT 
--                         0.2 * Avg(l_quantity) as avg, 
--                         agg_map_parallel(l_orderkey, l_linenumber, 0, 0) as mapped_agg
--                     FROM   lineitem
--                     WHERE  l_partkey = p_partkey
--             ) f
--             ON l_quantity < f.avg
--         WHERE  p_partkey = l_partkey
--             AND p_brand = 'Brand#23'
--             AND p_container = 'MED BOX') h
-- ) g;

-- using default substitutions
-- SELECT *, mark_later(mapped_agg_second) FROM (
--     SELECT 
--         SUM(l_extendedprice) / 7.0 AS avg_yearly, 
--         agg_map_parallel_second(p_partkey, l_orderkey, l_linenumber, 0) as mapped_agg_second,
--         sum(later)
--     FROM 
--         (SELECT 
--             *,
--             mark_later(mapped_agg) as later
--         FROM   part, lineitem
--             JOIN LATERAL (
--                     SELECT 
--                         0.2 * Avg(l_quantity) as avg, 
--                         agg_map_parallel(l_orderkey, l_linenumber, 0, 0) as mapped_agg
--                     FROM   lineitem
--                     WHERE  l_partkey = p_partkey
--             ) f
--             ON l_quantity < f.avg
--         WHERE  p_partkey = l_partkey
--             AND p_brand = 'Brand#23'
--             AND p_container = 'MED BOX') h
-- ) g;

