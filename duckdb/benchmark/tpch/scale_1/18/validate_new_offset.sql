-- using default substitutions
select
    c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice,
    sum(l_quantity)
from
    customer,
    orders,
    lineitem
where
    c_custkey = o_custkey
    and o_orderkey = l_orderkey
    and o_orderkey in (
        select
            l_orderkey
        from
            lineitem
        where lineitem.rowid in (
                select column_1_1
                from LAYER_1_%OUT_ID%
            )
        group by l_orderkey
    )
    and (customer.rowid, orders.rowid, lineitem.rowid) in (
        select
            (eval(column_1, column_1_1),
            column_2,
            column_4)
        FROM
            LAYER_3_%OUT_ID%
    )
group by
    c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice
order by
    o_totalprice desc,
    o_orderdate