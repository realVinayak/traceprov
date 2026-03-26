select n_name,
    sum(l_extendedprice * (1 - l_discount)) as revenue
from customer,
    orders,
    lineitem,
    supplier,
    nation,
    region
where (
        c_custkey,
        o_orderkey,
        l_orderkey,
        l_linenumber,
        s_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        select col_1,
            col_2,
            col_3,
            col_4,
            col_5,
            col_6,
            col_7
        from traceprov_relation_infer_1_mat
    )
group by n_name
order by revenue desc;