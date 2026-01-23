-- using default substitutions
select
    s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment
from
    part,
    supplier,
    partsupp,
    nation,
    region
where
    (
        p_partkey,
        s_suppkey,
        ps_partkey,
        ps_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        SELECT
            *
        from
            'LAYER_3'
    )
    and ps_supplycost = (
        select
            min(ps_supplycost)
        from
            partsupp,
            supplier,
            nation,
            region
        where
            p_partkey = ps_partkey
            and (
                ps_partkey,
                ps_suppkey,
                s_suppkey,
                n_nationkey,
                r_regionkey
            ) in (
                select
                    column_1_1,
                    column_2_1,
                    column_3_1,
                    column_4_1,
                    column_5_1
                from
                    'LAYER_1'
            )
    )
order by
    s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT
    100;