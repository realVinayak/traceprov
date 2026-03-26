select c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice,
    sum(l_quantity)
from customer,
    orders,
    lineitem
where o_orderkey in (
        select l_orderkey
        from lineitem
        where (l_orderkey, l_linenumber) in (
                select col_1,
                    col_2
                from traceprov_relation_infer_1_mat
            )
        group by l_orderkey
        having sum(l_quantity) > 300
    )
    and (c_custkey, o_orderkey, l_orderkey, l_linenumber) in (
        select col_1,
            col_2,
            col_3,
            col_4
        from traceprov_relation_infer_3_mat
    )
group by c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice
order by o_totalprice desc,
    o_orderdate
LIMIT 100;