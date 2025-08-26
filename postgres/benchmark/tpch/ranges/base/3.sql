SELECT *
FROM   (SELECT *
        FROM   (SELECT l_orderkey,
                       Sum(l_extendedprice * ( 1 - l_discount )) AS revenue,
                       o_orderdate,
                       o_shippriority
                FROM   lineitem,
                       orders,
                       customer
                WHERE  c_mktsegment = 'BUILDING'
                       AND c_custkey = o_custkey
                       AND l_orderkey = o_orderkey
                       AND o_orderdate < To_date('1995-03-15', 'YYYY-MM-DD')
                       AND l_shipdate > To_date('1995-03-15', 'YYYY-MM-DD')
                GROUP  BY l_orderkey,
                          o_orderdate,
                          o_shippriority) l2
        ORDER  BY revenue DESC,
                  o_orderdate) l1
LIMIT  10; 