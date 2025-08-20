-- using 1755693633 as a seed to the RNG


select
    s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment,
    log_subquery_pk(p_partkey, s_suppkey, n_nationkey, r_regionkey, ps_suppkey, ps_partkey),
    mark_later(mapped_agg)
from
    part,
    supplier,
    nation,
    region,
    partsupp
    JOIN LATERAL (
        select
            min(ps_supplycost) as min_ps_sc,
            agg_map_parallel(ps_suppkey, ps_partkey, s_suppkey, n_nationkey, r_regionkey) as mapped_agg
        from
            partsupp,
            supplier,
            nation,
            region
        where
            p_partkey = ps_partkey
            and s_suppkey = ps_suppkey
            and s_nationkey = n_nationkey
            and n_regionkey = r_regionkey
            and r_name = 'ASIA'
    ) as f
    on f.min_ps_sc = ps_supplycost
where
    p_partkey = ps_partkey
    and s_suppkey = ps_suppkey
    and p_size = 12
    and p_type like '%STEEL'
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
    and r_name = 'ASIA'
order by
    s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT 100;
