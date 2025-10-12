SELECT   c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice,
         SUM(l_quantity),
         agg_from_ptr_tag(mapped_agg)
FROM     customer,
         lineitem l2,
         orders
         JOIN  (
                  SELECT   
                    l1.l_orderkey,
                    agg_map_parallel_tag(l1.l_orderkey, l1.l_linenumber) as mapped_agg
                  FROM     lineitem l1
                  GROUP BY l_orderkey
                  HAVING   SUM(l_quantity) > 300 ) as f
        ON f.l_orderkey = o_orderkey
WHERE    c_custkey = o_custkey
AND      o_orderkey = l2.l_orderkey
GROUP BY c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice
ORDER BY o_totalprice DESC,
         o_orderdate
LIMIT 100;