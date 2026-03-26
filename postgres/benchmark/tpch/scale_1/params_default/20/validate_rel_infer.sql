select s_name,
    s_address
from supplier,
    nation
where s_suppkey in (
        select ps_suppkey
        from partsupp
        where ps_partkey in (
                select p_partkey
                from part
                where p_partkey in (
                        select col_0
                        from traceprov_relation_infer_1_mat
                    )
            )
            and ps_availqty > (
                select 0.5 * sum(l_quantity)
                from lineitem
                where l_partkey = ps_partkey
                    and l_suppkey = ps_suppkey
                    and (l_orderkey, l_linenumber) in (
                        select col_5,
                            col_6
                        from traceprov_relation_infer_2_mat
                    )
            )
            and (ps_partkey, ps_suppkey) in (
                select col_0,
                    col_1
                from traceprov_relation_infer_4_mat
            )
    )
    and (s_suppkey, n_nationkey) in (
        select col_0,
            col_1
        from traceprov_relation_infer_5_mat
    )
order by s_name;