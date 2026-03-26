select o_orderpriority,
    count(*) as order_count
from orders
where o_orderkey in (
        select col_1
        from traceprov_relation_infer_2_mat
    )
    and exists (
        select *
        from lineitem
        where l_orderkey = o_orderkey
            and (l_orderkey, l_linenumber) in (
                select col_3,
                    col_4
                from traceprov_relation_infer_1_mat
            )
    )
group by o_orderpriority
order by o_orderpriority;