SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM   lineitem l1,
       part,
       (SELECT 0.2 * Avg(l_quantity) AS avgl,
               l_partkey
        FROM   lineitem
        GROUP  BY l_partkey) l2
WHERE  p_partkey = l1.l_partkey
       AND p_brand = 'Brand#31'
       AND p_container = 'SM PKG'
       AND l2.l_partkey = p_partkey
       AND l_quantity < avgl; 