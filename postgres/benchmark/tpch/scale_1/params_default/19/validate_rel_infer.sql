select sum(l_extendedprice * (1 - l_discount)) as revenue
from lineitem,
    part
where (l_orderkey, l_linenumber, p_partkey) in (
        select col_1,
            col_2,
            col_3
        from traceprov_relation_infer_1_mat
    );