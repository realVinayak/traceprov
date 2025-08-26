SELECT *
FROM   (SELECT n_name,
               SUM(l_extendedprice * ( 1 - l_discount )) AS revenue
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
               AND o_orderdate >= To_date('1994-01-01', 'YYYY-MM-DD')
               AND o_orderdate < To_date('1995-01-01', 'YYYY-MM-DD')
        GROUP  BY n_name) AS sub
ORDER  BY revenue DESC; 