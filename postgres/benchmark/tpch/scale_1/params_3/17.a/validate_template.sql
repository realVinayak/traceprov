-- using 1755708640 as a seed to the RNG


SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM
    lineitem,
    part
WHERE  p_partkey = l_partkey
    AND p_brand = 'Brand#14'
    AND p_container = 'WRAP PKG'
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

