-- using default substitutions
PROVENANCE OF (
    select s_acctbal,
        s_name,
        n_name,
        p_partkey,
        p_mfgr,
        s_address,
        s_phone,
        s_comment
    from part USE PROVENANCE (p_partkey),
        supplier USE PROVENANCE (s_suppkey),
        partsupp USE PROVENANCE (ps_partkey),
        nation USE PROVENANCE (n_nationkey),
        region USE PROVENANCE (r_regionkey)
    where p_partkey = ps_partkey
        and s_suppkey = ps_suppkey
        and p_size = 15
        and p_type like '%BRASS'
        and s_nationkey = n_nationkey
        and n_regionkey = r_regionkey
        and r_name = 'EUROPE'
        and ps_supplycost = (
            select min(ps_supplycost)
            from supplier USE PROVENANCE (s_suppkey),
                partsupp USE PROVENANCE (ps_partkey),
                nation USE PROVENANCE (n_nationkey),
                region USE PROVENANCE (r_regionkey)
            where p_partkey = ps_partkey
                and s_suppkey = ps_suppkey
                and s_nationkey = n_nationkey
                and n_regionkey = r_regionkey
                and r_name = 'EUROPE'
        )
    order by s_acctbal desc,
        n_name,
        s_name,
        p_partkey
    LIMIT 100
);