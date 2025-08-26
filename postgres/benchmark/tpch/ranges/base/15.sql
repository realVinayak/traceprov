SELECT s_suppkey,
       s_name,
       s_address,
       s_phone,
       total_revenue
FROM   supplier,
       (SELECT supplier_no,
               total_revenue
        FROM   (SELECT l_suppkey                                 AS supplier_no,
                       SUM(l_extendedprice * ( 1 - l_discount )) AS
                       total_revenue
                FROM   lineitem
                WHERE  l_shipdate >= '1996-12-01'
                       AND l_shipdate < '1997-03-01'
                GROUP  BY l_suppkey) l1,
               (SELECT Max(total_revenue) AS total_revenue2
                FROM   (SELECT l_suppkey                                 AS
                               supplier_no
                               ,
                               SUM(l_extendedprice * ( 1
                                   - l_discount )) AS total_revenue
                        FROM   lineitem
                        WHERE  l_shipdate >= '1996-12-01'
                               AND l_shipdate < '1997-03-01'
                        GROUP  BY l_suppkey) l2) l3
        WHERE  l1.total_revenue = l3.total_revenue2) l5
WHERE  s_suppkey = supplier_no
ORDER  BY s_suppkey; 