-- using default substitutions


SELECT SUM(l_extendedprice) / 7.0 AS avg_yearly
FROM
    lineitem,
    part
WHERE  p_partkey = l_partkey
    AND p_brand = 'Brand#23'
    AND p_container = 'MED BOX'
    AND p_partkey in (select p_partkey from layer_0)
    AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
    AND l_quantity < (
        select
            0.2 * avg(l_quantity)
        from
            lineitem
        where
            l_partkey = p_partkey
    );

