-- using 1755709841 as a seed to the RNG


SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM   
       part, 
       lineitem
           JOIN LATERAL (
            SELECT 
                0.2 * Avg(l2.l_quantity) as avg
            FROM   lineitem l2
            WHERE  l2.l_partkey = p_partkey
            AND (l2.l_orderkey, l2.l_linenumber) in (%A%)
    ) f
    ON l_quantity < f.avg
WHERE  p_partkey = l_partkey
    AND p_brand = 'Brand#22'
    AND p_container = 'LG CASE'
    AND (l_orderkey, l_linenumber) in (%A%);
