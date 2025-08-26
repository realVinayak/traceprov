SELECT *
FROM   (SELECT *
        FROM   (SELECT c_custkey,
                       c_name,
                       Sum(l_extendedprice * ( 1 - l_discount )) AS revenue,
                       c_acctbal,
                       n_name,
                       c_address,
                       c_phone,
                       c_comment
                FROM   customer,
                       orders,
                       lineitem,
                       nation
                WHERE  c_custkey = o_custkey
                       AND l_orderkey = o_orderkey
                       AND o_orderdate >= To_date('1994-12-01', 'YYYY-MM-DD')
                       AND o_orderdate < To_date('1995-03-01', 'YYYY-MM-DD')
                       AND l_returnflag = 'R'
                       AND c_nationkey = n_nationkey
                GROUP  BY c_custkey,
                          c_name,
                          c_acctbal,
                          c_phone,
                          n_name,
                          c_address,
                          c_comment) o1
        ORDER  BY revenue DESC) o2
LIMIT  20; 