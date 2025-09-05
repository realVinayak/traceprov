-- using 1755708640 as a seed to the RNG


SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM
    lineitem,
    part
WHERE  p_partkey = l_partkey
    AND p_brand = 'Brand#14'
    AND p_container = 'WRAP PKG'
    AND p_partkey in (SELECT group_outer.p_partkey FROM group_outer)
    AND (l_orderkey, l_linenumber) in (SELECT group_outer.l_orderkey,group_outer.l_linenumber FROM group_outer)
    AND l_quantity < (
        select
            0.2 * avg(l_quantity)
        from
            lineitem
        where
            l_partkey = p_partkey
            AND (l_orderkey, l_linenumber) in (%A%)
    );

