-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM
(SELECT   s_name,
         Count(*) AS numwait,
         agg_map_parallel(
            s_suppkey,
            l1.l_orderkey,
            l1.l_linenumber,
            o_orderkey,
            n_nationkey
         ) as mapped_agg
FROM     supplier,
         lineitem l1,
         orders,
         nation
WHERE    s_suppkey = l1.l_suppkey
AND      o_orderkey = l1.l_orderkey
AND      o_orderstatus = 'F'
AND      l1.l_receiptdate > l1.l_commitdate
AND      EXISTS
         (
                SELECT *
                FROM   lineitem l2
                WHERE  l2.l_orderkey = l1.l_orderkey
                AND    l2.l_suppkey <> l1.l_suppkey 
                AND    log_subquery_pk(l2.l_orderkey, l2.l_linenumber)
                )
AND      NOT EXISTS
         (
                SELECT *
                FROM   lineitem l3
                WHERE  (l3.l_orderkey = l1.l_orderkey
                AND    l3.l_suppkey <> l1.l_suppkey
                AND    l3.l_receiptdate > l3.l_commitdate)
                OR    log_subquery_pk_neg(l3.l_orderkey, l3.l_linenumber)
                 )
AND      s_nationkey = n_nationkey
AND      n_name = 'SAUDI ARABIA'
GROUP BY s_name
) f
ORDER BY numwait DESC,
         s_name
LIMIT 100;