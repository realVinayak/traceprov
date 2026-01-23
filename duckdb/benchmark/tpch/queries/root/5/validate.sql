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
    (
        c_custkey,
        o_orderkey,
        l_orderkey,
        l_linenumber,
        s_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        select
            column_1,
            column_2,
            column_3,
            column_4,
            column_5,
            column_6,
            column_7
        from
            'LAYER_1'
    )
group by
    n_name
order by
    revenue desc;