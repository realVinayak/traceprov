-- ['l_orderkey', 'o_orderkey', 'c_custkey']
EXPLAIN SELECT *, mapped_agg FROM
    (SELECT   l_orderkey,
            SUM(l_extendedprice * (1 - l_discount)) AS revenue,
            o_orderdate,
            o_shippriority,
            agg_map_parallel(l_orderkey, o_orderkey, c_custkey) as mapped_agg
    FROM     customer,
            orders,
            lineitem
    WHERE    c_mktsegment = 'BUILDING'
    AND      c_custkey = o_custkey
    AND      l_orderkey = o_orderkey
    AND      o_orderdate < DATE '1995-03-15'
    AND      l_shipdate >  DATE '1995-03-15'
    GROUP BY l_orderkey,
            o_orderdate,
            o_shippriority
    ) f
ORDER BY revenue DESC,
         o_orderdate
LIMIT 10;
