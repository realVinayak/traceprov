select l_orderkey,
    sum(l_extendedprice * (1 - l_discount)) as revenue,
    o_orderdate,
    o_shippriority
from customer,
    orders,
    lineitem
where (c_custkey, o_orderkey, l_orderkey, l_linenumber) in (
        select col_1,
            col_2,
            col_3,
            col_4
        from traceprov_relation_infer_1_mat
    )
group by l_orderkey,
    o_orderdate,
    o_shippriority
order by revenue desc,
    o_orderdate;