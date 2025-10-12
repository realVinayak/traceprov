SELECT *, mark_later(mapped_agg) FROM (
    SELECT n_name,
        SUM(l_extendedprice * ( 1 - l_discount )) AS revenue,
        agg_map_parallel(
            c_custkey,
            o_orderkey,
            l_orderkey,
            l_linenumber,
            s_suppkey,
            n_nationkey,
            r_regionkey
        ) as mapped_agg
    FROM   customer,
        orders,
        lineitem,
        supplier,
        nation,
        region
    WHERE  c_custkey = o_custkey
        AND l_orderkey = o_orderkey
        AND l_suppkey = s_suppkey
        AND c_nationkey = s_nationkey
        AND s_nationkey = n_nationkey
        AND n_regionkey = r_regionkey
        AND r_name = 'ASIA'
        AND o_orderdate >= DATE '1994-01-01'
        AND o_orderdate < DATE '1994-01-01' + interval '1' year
    GROUP  BY n_name
) f
ORDER  BY revenue DESC; 