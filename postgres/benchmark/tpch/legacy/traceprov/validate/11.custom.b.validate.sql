-- using CUSTOM substitutions
SELECT ps_partkey,
       SUM(ps_supplycost * ps_availqty) AS value
FROM   partsupp,
       supplier,
       nation
WHERE  ps_suppkey = s_suppkey
       AND s_nationkey = n_nationkey
       AND n_name = 'GERMANY'
       AND (ps_partkey, ps_suppkey) in (SELECT group_outer.ps_partkey,group_outer.ps_suppkey FROM group_outer)
       AND s_suppkey in (SELECT group_outer.s_suppkey FROM group_outer)
       AND n_nationkey in (SELECT group_outer.n_nationkey FROM group_outer)
GROUP  BY ps_partkey
HAVING SUM(ps_supplycost * ps_availqty) > (SELECT
       SUM(ps_supplycost * ps_availqty) * 0.0000100000
                                           FROM   partsupp,
                                                  supplier,
                                                  nation
                                           WHERE  ps_suppkey = s_suppkey
                                                  AND s_nationkey = n_nationkey
                                                  AND n_name = 'INDIA'
                                                  AND (ps_suppkey, ps_partkey) in (%A%)
                                                  AND s_suppkey in (%B%)
                                                  AND n_nationkey in (%C%)
                                                  )
ORDER  BY value DESC;