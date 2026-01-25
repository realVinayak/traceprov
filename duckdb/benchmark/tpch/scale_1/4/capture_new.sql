-- using default substitutions
select
    *,
    traceprov_log_entry_1 (3, mapped_agg)
FROM
    (
        select
            o_orderpriority,
            count(*) as order_count,
            traceprov_agg_key_parallel_offset_1 (2, orders.rowid) as mapped_agg
        from
            orders
        where
            o_orderdate >= date '1993-07-01'
            and o_orderdate < date '1993-07-01' + interval '3' month
            and exists (
                select
                    1
                from
                    lineitem
                where
                    l_orderkey = o_orderkey
                    and l_commitdate < l_receiptdate
                    AND CASE
                        WHEN (
                            (lineitem.l_orderkey = orders.o_orderkey)
                            AND (lineitem.l_commitdate < lineitem.l_receiptdate)
                        ) THEN traceprov_log_entry_2 (1, (orders.rowid), (lineitem.rowid))
                        ELSE false
                    END
            )
        group by
            o_orderpriority
        order by
            o_orderpriority
    ) F