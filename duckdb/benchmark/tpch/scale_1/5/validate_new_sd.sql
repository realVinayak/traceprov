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
            opid_13_customer,
            opid_11_orders,
            opid_9_lineitem,
            opid_17_supplier,
            opid_15_nation,
            opid_16_region
        from
            LAYER_1
    )
group by
    n_name
order by
    revenue desc;