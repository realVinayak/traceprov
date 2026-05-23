-- using default substitutions
select n_name,
    sum(l_extendedprice * (1 - l_discount)) as revenue
from customer,
    orders,
    lineitem,
    supplier,
    nation,
    region
where c_custkey = o_custkey
    and l_orderkey = o_orderkey
    and l_suppkey = s_suppkey
    and c_nationkey = s_nationkey
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
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
    and supplier.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 11
    )
    and nation.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 3
    )
    and region.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 4
    )
group by n_name
order by revenue desc;