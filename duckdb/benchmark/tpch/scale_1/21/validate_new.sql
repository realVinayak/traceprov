-- using default substitutions
select s_name,
    count(*) as numwait
from supplier,
    lineitem l1,
    orders,
    nation
where exists (
        select *
        from lineitem l2
        where l2.l_orderkey = l1.l_orderkey
            and (
                l2.rowid in (
                    select column_1_1
                    from traceprov_lineage_1
                )
            )
    )
    and not exists (
        select *
        from lineitem l3
        where l3.l_orderkey = l1.l_orderkey
            and l3.l_suppkey <> l1.l_suppkey
            and l3.l_receiptdate > l3.l_commitdate
    )
    and (
        supplier.rowid,
        l1.rowid,
        orders.rowid,
        nation.rowid
    ) in (
        select (
                column_1,
                column_2,
                column_3,
                column_4
            )
        from traceprov_lineage_2
    )
    and s_suppkey = l1.l_suppkey
    and o_orderkey = l1.l_orderkey
group by s_name
order by numwait desc,
    s_name
LIMIT 100