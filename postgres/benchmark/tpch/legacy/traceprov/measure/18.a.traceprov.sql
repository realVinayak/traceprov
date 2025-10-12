-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM (
    SELECT   c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice,
         SUM(l_quantity),
         agg_map_parallel(c_custkey, o_orderkey, l_orderkey, l_linenumber) as mapped_agg
FROM     customer,
         orders,
         lineitem
WHERE    o_orderkey IN
         (
                  SELECT   l_orderkey
                  FROM     lineitem
                  GROUP BY l_orderkey
                  HAVING   SUM(l_quantity) > 300 )
AND      c_custkey = o_custkey
AND      o_orderkey = l_orderkey
GROUP BY c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice
) f
ORDER BY o_totalprice DESC,
         o_orderdate
LIMIT 100;