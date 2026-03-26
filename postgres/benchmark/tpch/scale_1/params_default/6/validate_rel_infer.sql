select sum(l_extendedprice * l_discount) as revenue
from lineitem
where (l_orderkey, l_linenumber) in (
        select col_1,
            col_2
        from traceprov_relation_infer_1_mat
    );