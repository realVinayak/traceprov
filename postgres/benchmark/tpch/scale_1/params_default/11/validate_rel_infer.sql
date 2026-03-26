select ps_partkey,
    sum(ps_supplycost * ps_availqty) as value
from partsupp,
    supplier,
    nation
where (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
        select col_1,
            col_2,
            col_3,
            col_4
        from traceprov_relation_infer_3_mat
    )
group by ps_partkey
having sum(ps_supplycost * ps_availqty) > (
        select sum(ps_supplycost * ps_availqty) * 0.0001000000
        from partsupp,
            supplier,
            nation
        where (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
                select col_1,
                    col_2,
                    col_3,
                    col_4
                from traceprov_relation_infer_1_mat
            )
    )
order by value desc;