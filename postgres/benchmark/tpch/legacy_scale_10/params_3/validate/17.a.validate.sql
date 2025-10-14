-- using 1755709849 as a seed to the RNG


SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM
    lineitem,
    part
WHERE  p_partkey = l_partkey
    AND p_brand = 'Brand#22'
    AND p_container = 'JUMBO PKG'
    AND p_partkey in (%A%)
    AND (l_orderkey, l_linenumber) in (%B%)
    AND l_quantity < (
        select
            0.2 * avg(l_quantity)
        from
            lineitem
        where
            l_partkey = p_partkey
    );

