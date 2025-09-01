-- using default substitutions
SELECT   s_acctbal,
         s_name,
         n_name,
         p_partkey,
         p_mfgr,
         s_address,
         s_phone,
         s_comment,
         traceprov_log_subquery_pk(4, p_partkey, s_suppkey, n_nationkey, r_regionkey, ps_suppkey, ps_partkey),
         mark_later(mapped_agg)
FROM     part,
         supplier,
         nation,
         region,
         partsupp
         JOIN LATERAL  (
                SELECT 
                    Min(ps_supplycost) as min_ps_sc,
                    traceprov_agg_key_parallel(1, ps_suppkey, ps_partkey, s_suppkey, n_nationkey, r_regionkey) as mapped_agg
                FROM   partsupp,
                       supplier,
                       nation,
                       region
                WHERE  p_partkey = ps_partkey
                AND    s_suppkey = ps_suppkey
                AND    s_nationkey = n_nationkey
                AND    n_regionkey = r_regionkey
                AND    r_name = 'EUROPE' ) as f
         on f.min_ps_sc = ps_supplycost
WHERE    p_partkey = ps_partkey
AND      s_suppkey = ps_suppkey
AND      p_size = 15
AND      p_type LIKE '%BRASS'
AND      s_nationkey = n_nationkey
AND      n_regionkey = r_regionkey
AND      r_name = 'EUROPE'
ORDER BY s_acctbal DESC,
         n_name,
         s_name,
         p_partkey 
LIMIT 100;