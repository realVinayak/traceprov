select s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment
from part,
    supplier,
    partsupp,
    nation,
    region
where (
        p_partkey,
        s_suppkey,
        ps_partkey,
        ps_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        select col_0,
            col_1,
            col_2,
            col_3,
            col_4,
            col_5
        from traceprov_relation_infer_3_mat
    )
    and ps_supplycost = (
        select min(ps_supplycost)
        from partsupp,
            supplier,
            nation,
            region
        where p_partkey = ps_partkey
            and (
                ps_partkey,
                ps_suppkey,
                s_suppkey,
                n_nationkey,
                r_regionkey
            ) in (
                select col_8,
                    col_9,
                    col_10,
                    col_11,
                    col_12
                from traceprov_relation_infer_1_mat
            )
    )
order by s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT 100;