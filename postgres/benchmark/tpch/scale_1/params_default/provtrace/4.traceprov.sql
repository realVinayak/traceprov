-- using default substitutions

SELECT *, mark_later(mapped_agg)
FROM 
(
    SELECT o_orderpriority,
       Count(*) AS order_count,
       traceprov_agg_key_parallel(1, o_orderkey) as mapped_agg
    FROM   orders
    WHERE  o_orderdate >= DATE '1993-07-01'
        AND o_orderdate < DATE '1993-07-01' + interval '3' month
        AND EXISTS (SELECT *
                    FROM   lineitem
                    WHERE  l_orderkey = o_orderkey
                            AND l_commitdate < l_receiptdate
                            AND traceprov_log_subquery_pk(4, l_orderkey, l_linenumber, o_orderkey)
                        )
    GROUP  BY o_orderpriority
) f
ORDER  BY o_orderpriority; 