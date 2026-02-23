-- using default substitutions
select
    c_custkey,
    c_name,
    sum(l_extendedprice * (1 - l_discount)) as revenue,
    c_acctbal,
    n_name,
    c_address,
    c_phone,
    c_comment
from
    customer,
    orders,
    lineitem,
    nation
where
    c_custkey = o_custkey
    and l_orderkey = o_orderkey
    and c_nationkey = n_nationkey
    and (
        customer.rowid,
        orders.rowid,
        lineitem.rowid,
        nation.rowid
    ) in (
        select
            column_1,
            column_2,
            column_3,
            column_4
        from
            LAYER_1_19
    )
group by
    c_custkey,
    c_name,
    c_acctbal,
    c_phone,
    n_name,
    c_address,
    c_comment
order by
    revenue desc
LIMIT
    20;