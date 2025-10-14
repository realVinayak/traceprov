SELECT   c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice,
         SUM(l_quantity)
FROM     customer,
         orders,
         lineitem
WHERE    o_orderkey IN
         (
                  SELECT   l_orderkey
                  FROM     lineitem
                  WHERE    (l_orderkey, l_linenumber) in (%A%)
                  GROUP BY l_orderkey
                  HAVING   SUM(l_quantity) > 300 )
AND      c_custkey = o_custkey
AND      o_orderkey = l_orderkey
AND      c_custkey in (SELECT distinct captured_id.c_custkey FROM captured_id)
AND      o_orderkey in (SELECT distinct captured_id.o_orderkey FROM captured_id)
AND      (l_orderkey, l_linenumber) in (SELECT distinct captured_id.l_orderkey,captured_id.l_linenumber FROM captured_id)
GROUP BY c_name,
         c_custkey,
         o_orderkey,
         o_orderdate,
         o_totalprice
ORDER BY o_totalprice DESC,
         o_orderdate
LIMIT 100;