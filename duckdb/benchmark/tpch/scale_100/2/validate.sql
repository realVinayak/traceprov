-- using default substitutions
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
where p_partkey = ps_partkey
    and s_suppkey = ps_suppkey
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
    and ps_supplycost = (
        select min(ps_supplycost)
        from partsupp,
            supplier,
            nation,
            region
        where p_partkey = ps_partkey
            and s_suppkey = ps_suppkey
            and s_nationkey = n_nationkey
            and n_regionkey = r_regionkey
            and (
                ps_partkey,
                ps_suppkey,
                s_suppkey,
                n_nationkey,
                r_regionkey
            ) in (
                select (
                        column_1_1,
                        column_2_1,
                        column_3_1,
                        column_4_1,
                        column_5_1
                    )
                from traceprov_lineage_1
            )
    )
    and (
        p_partkey,
        s_suppkey,
        ps_partkey,
        ps_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        SELECT (
                column_0,
                column_1,
                column_2,
                column_3,
                column_4,
                column_5
            )
        from traceprov_lineage_3
    )
order by s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT 100;