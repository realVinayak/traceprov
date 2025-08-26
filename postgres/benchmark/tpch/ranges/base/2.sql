SELECT s_acctbal,
       s_name,
       n_name,
       p_partkey,
       p_mfgr,
       s_address,
       s_phone,
       s_comment
FROM   part,
       supplier,
       partsupp ps,
       nation,
       region,
       (SELECT Min(ps_supplycost) AS min_supplycost,
               ps_partkey
        FROM   partsupp,
               supplier,
               nation,
               region
        WHERE  s_suppkey = ps_suppkey
               AND s_nationkey = n_nationkey
               AND n_regionkey = r_regionkey
               AND r_name = 'EUROPE'
        GROUP  BY ps_partkey) m
WHERE  p_partkey = ps.ps_partkey
       AND s_suppkey = ps.ps_suppkey
       AND p_size = 15
       AND p_type LIKE '%BRASS'
       AND s_nationkey = n_nationkey
       AND n_regionkey = r_regionkey
       AND r_name = 'EUROPE'
       AND ps_supplycost = m.min_supplycost
       AND p_partkey = m.ps_partkey
ORDER  BY s_acctbal DESC,
          n_name,
          s_name,
          p_partkey
LIMIT  100; 