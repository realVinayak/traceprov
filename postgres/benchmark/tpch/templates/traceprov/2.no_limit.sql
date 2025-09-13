-- $ID$
-- TPC-H/TPC-R Minimum Cost Supplier Query (Q2)
-- Functional Query Definition
-- Approved February 1998
:x
:o
select
    s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment,
    traceprov_log_subquery_pk(4, p_partkey, s_suppkey, n_nationkey, r_regionkey, ps_suppkey, ps_partkey),
    mark_later(mapped_agg)
from
    part,
    supplier,
    nation,
    region,
    partsupp
    JOIN LATERAL (
        select
            min(ps_supplycost)  as min_ps_sc,
            traceprov_agg_key_parallel(1, ps_suppkey, ps_partkey, s_suppkey, n_nationkey, r_regionkey) as mapped_agg
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
            and r_name = ':3'
    ) as f
    ON f.min_ps_sc = ps_supplycost
where
    p_partkey = ps_partkey
    and s_suppkey = ps_suppkey
    and p_size = :1
    and p_type like '%:2'
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
    and r_name = ':3'
order by
    s_acctbal desc,
    n_name,
    s_name,
    p_partkey;
