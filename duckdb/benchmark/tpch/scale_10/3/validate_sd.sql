-- using default substitutions
select l_orderkey,
    sum(l_extendedprice * (1 - l_discount)) as revenue,
    o_orderdate,
    o_shippriority
from customer,
    orders,
    lineitem
where c_custkey = o_custkey
    and l_orderkey = o_orderkey
    and customer.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 2
    )
    and orders.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 1
    )
    and lineitem.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 0
    )
group by l_orderkey,
    o_orderdate,
    o_shippriority
order by revenue desc,
    o_orderdate
LIMIT 10;