-- using default substitutions
select
    *,
    traceprov_log_entry_1 (4, mapped_agg)
FROM
    (
        select
            c_name,
            c_custkey,
            o_orderkey,
            o_orderdate,
            o_totalprice,
            sum(l_quantity),
            traceprov_agg_key_parallel_offset_3 (3, customer.rowid, orders.rowid, lineitem.rowid) as mapped_agg
        from
            customer,
            orders,
            lineitem
        where
            o_orderkey in (
                select
                    l_orderkey
                from
                    (
                        select
                            l_orderkey,
                            traceprov_log_entry_volatile_1 (
                                2,
                                traceprov_agg_key_parallel_offset_1 (1, lineitem.rowid)
                            )
                        from
                            lineitem
                        group by
                            l_orderkey
                        having
                            sum(l_quantity) > 300
                    ) f
            )
            and c_custkey = o_custkey
            and o_orderkey = l_orderkey
        group by
            c_name,
            c_custkey,
            o_orderkey,
            o_orderdate,
            o_totalprice
        order by
            o_totalprice desc,
            o_orderdate
        LIMIT
            100
    ) F;