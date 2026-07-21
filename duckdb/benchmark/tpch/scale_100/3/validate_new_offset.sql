-- using default substitutions
select
    l_orderkey,
    sum(l_extendedprice * (1 - l_discount)) as revenue,
    o_orderdate,
    o_shippriority
from
    customer,
    orders,
    lineitem
where
    c_custkey = o_custkey
    and l_orderkey = o_orderkey
    and (customer.rowid, orders.rowid, lineitem.rowid) in (
        select
            (eval(column_1, column_1_1),
            column_2,
            column_3)
        from
            LAYER_1_%OUT_ID%
    )
group by
    l_orderkey,
    o_orderdate,
    o_shippriority
order by
    revenue desc,
    o_orderdate
LIMIT
    10;