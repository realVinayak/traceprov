select p_brand,
    p_type,
    p_size,
    count(distinct ps_suppkey) as supplier_cnt
from partsupp,
    part
where (ps_partkey, ps_suppkey, p_partkey) in (
        select col_1,
            col_2,
            col_3
        from traceprov_relation_infer_1_mat
    )
group by p_brand,
    p_type,
    p_size
order by supplier_cnt desc,
    p_brand,
    p_type,
    p_size;