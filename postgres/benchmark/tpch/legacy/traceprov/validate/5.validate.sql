SELECT n_name,
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
       AND o_orderdate >= DATE '1994-01-01'
       AND o_orderdate < DATE '1994-01-01' + interval '1' year
       AND c_custkey in (%A%)
       AND o_orderkey in (%B%)
       AND (l_orderkey, l_linenumber) in (%C%)
       AND s_suppkey in (%D%)
       AND n_nationkey in (%E%)
       AND r_regionkey in (%F%)
GROUP  BY n_name
ORDER  BY revenue DESC; 