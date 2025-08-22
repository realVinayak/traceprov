SELECT *, mark_later(mapped_agg) FROM (SELECT ps_partkey,
       Sum(ps_supplycost * ps_availqty) AS value,
       agg_map_parallel(ps_partkey, ps_suppkey, s_suppkey, n_nationkey) as mapped_agg
FROM   partsupp,
       supplier,
       nation
WHERE  ps_suppkey = s_suppkey
       AND s_nationkey = n_nationkey
       AND n_name = 'GERMANY'
GROUP  BY ps_partkey
HAVING Sum(ps_supplycost * ps_availqty) > (SELECT
       Sum(ps_supplycost * ps_availqty) * 0.000001
                                           FROM   partsupp,
                                                  supplier,
                                                  nation
                                           WHERE  ps_suppkey = s_suppkey
                                                  AND s_nationkey = n_nationkey
                                                  AND n_name = 'GERMANY')
) f
ORDER  BY value DESC;

SELECT *, mark_later(mapped_agg) FROM (SELECT 
    SUM(ps_supplycost * ps_availqty) * 0.000001 as summed,
    agg_map_parallel(ps_partkey, ps_suppkey, s_suppkey, n_nationkey) as mapped_agg
FROM   partsupp,
       supplier,
       nation
WHERE  ps_suppkey = s_suppkey
       AND s_nationkey = n_nationkey
       AND n_name = 'GERMANY') f;