select sum(l_extendedprice) / 7.0 as avg_yearly
from lineitem,
    part
where (l_orderkey, l_linenumber, p_partkey) in (
        select col_1,
            col_2,
            col_3
        from traceprov_relation_infer_3_mat
    )
    and l_quantity < (
        select 0.2 * avg(l_quantity)
        from lineitem
        where (p_partkey, l_orderkey, l_linenumber) in (
                select col_4,
                    col_6,
                    col_7
                from traceprov_relation_infer_1_mat
            )
    );