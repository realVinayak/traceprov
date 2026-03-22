-- using default substitutions
select *,
    traceprov_log_entry_1 (3, mapped_agg)
FROM (
        select s_name,
            count(*) as numwait,
            traceprov_agg_key_parallel_offset_4 (
                2,
                supplier.rowid,
                l1.rowid,
                orders.rowid,
                nation.rowid
            ) as mapped_agg
        from supplier,
            lineitem l1,
            orders,
            nation
        where s_suppkey = l1.l_suppkey
            and o_orderkey = l1.l_orderkey
            and o_orderstatus = 'F'
            and l1.l_receiptdate > l1.l_commitdate
            and exists (
                select 1
                from lineitem l2
                where l2.l_orderkey = l1.l_orderkey
                    and l2.l_suppkey <> l1.l_suppkey
                    AND CASE
                        WHEN (
                            (l2.l_orderkey = l1.l_orderkey)
                            AND (l2.l_suppkey <> l1.l_suppkey)
                        ) THEN traceprov_log_entry_bool_2 (1, (l1.rowid), (l2.rowid))
                        ELSE false
                    END
            )
            and not exists (
                select 1
                from lineitem l3
                where l3.l_orderkey = l1.l_orderkey
                    and l3.l_suppkey <> l1.l_suppkey
                    and l3.l_receiptdate > l3.l_commitdate
            )
            and s_nationkey = n_nationkey
            and n_name = 'SAUDI ARABIA'
        group by s_name
        order by numwait desc,
            s_name
        LIMIT 100
    ) F;