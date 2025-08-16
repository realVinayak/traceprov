select *, mark_later(mapped_agg) FROM (SELECT   o_orderpriority,
         Count(*) AS order_count,
         agg_map_parallel(o_orderkey) as mapped_agg
FROM     orders
WHERE    o_orderdate >= DATE '1993-05-01'
AND      o_orderdate <  DATE '1993-05-01' + interval '3' month
AND      EXISTS
         (
                SELECT *
                FROM   lineitem
                WHERE  l_orderkey = o_orderkey
                AND    l_commitdate < l_receiptdate
                AND    log_subquery_pk(l_orderkey, l_linenumber, o_orderkey)
                 )
GROUP BY o_orderpriority
ORDER BY o_orderpriority limit 1) f;