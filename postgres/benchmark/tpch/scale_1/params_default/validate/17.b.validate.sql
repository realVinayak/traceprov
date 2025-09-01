SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM   lineitem,
       part
WHERE  p_partkey = l_partkey
       AND p_brand = 'Brand#23'
       AND p_container = 'MED BOX'
       AND p_partkey in (SELECT group_outer.p_partkey FROM group_outer)
       AND (l_orderkey, l_linenumber) in (SELECT group_outer.l_orderkey,group_outer.l_linenumber FROM group_outer)
       AND l_quantity < (SELECT 0.2 * Avg(l_quantity)
                         FROM   lineitem
                         WHERE  l_partkey = p_partkey
                         AND    (l_orderkey, l_linenumber) in (%A%)
                         );