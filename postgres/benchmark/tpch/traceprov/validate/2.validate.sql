-- using default substitutions
SELECT   s_acctbal,
         s_name,
         n_name,
         p_partkey,
         p_mfgr,
         s_address,
         s_phone,
         s_comment
FROM     part,
         supplier,
         partsupp,
         nation,
         region
WHERE    p_partkey = ps_partkey
AND      s_suppkey = ps_suppkey
AND      p_size = 15
AND      p_type LIKE '%BRASS'
AND      s_nationkey = n_nationkey
AND      n_regionkey = r_regionkey
AND      r_name = 'EUROPE'
AND      ps_supplycost =
         (
                SELECT Min(ps_supplycost)
                FROM   partsupp,
                       supplier,
                       nation,
                       region
                WHERE  p_partkey = ps_partkey
                AND    s_suppkey = ps_suppkey
                AND    s_nationkey = n_nationkey
                AND    n_regionkey = r_regionkey
                AND    r_name = 'EUROPE' 
                AND    (ps_suppkey, ps_partkey) in (%F%)
                AND    s_suppkey in (%G%)
                AND    n_nationkey in (%H%)
                AND    r_regionkey in (%I%)
                )
AND      p_partkey in (%A%)
AND      s_suppkey in (%B%)
AND      n_nationkey in (%C%)
AND      r_regionkey in (%D%)
AND      (ps_partkey, ps_suppkey) in (%E%)
ORDER BY s_acctbal DESC,
         n_name,
         s_name,
         p_partkey 
LIMIT 100;