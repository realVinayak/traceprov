-- using default substitutions
select
    n_name,
    sum(l_extendedprice * (1 - l_discount)) as revenue
from
    customer,
    orders,
    lineitem,
    supplier,
    nation,
    region
where
    c_custkey = o_custkey
    and l_orderkey = o_orderkey
    and l_suppkey = s_suppkey
    and c_nationkey = s_nationkey
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
    and (
        customer.rowid,
        orders.rowid,
        lineitem.rowid,
        supplier.rowid,
        nation.rowid,
        region.rowid
    ) in (
        select
            (eval(column_1, column_1_1),
            column_2,
            column_3,
            column_4,
            column_5,
            column_6)
        from
            LAYER_1_%OUT_ID%
    )
group by
    n_name
order by
    revenue desc;