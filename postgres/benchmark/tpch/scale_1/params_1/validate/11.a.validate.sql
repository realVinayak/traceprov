-- using 1755693633 as a seed to the RNG


SELECT ps_partkey,
       SUM(ps_supplycost * ps_availqty) AS value
FROM   partsupp,
       supplier,
       nation
WHERE  ps_suppkey = s_suppkey
       AND s_nationkey = n_nationkey
       AND n_name = 'GERMANY'
       AND (ps_partkey, ps_suppkey) in (%A%)
       AND s_suppkey in (%B%)
       AND n_nationkey in (%C%)
GROUP  BY ps_partkey
HAVING SUM(ps_supplycost * ps_availqty) > (SELECT
       SUM(ps_supplycost * ps_availqty) * 0.0001000000
                                           FROM   partsupp,
                                                  supplier,
                                                  nation
                                           WHERE  ps_suppkey = s_suppkey
                                                  AND s_nationkey = n_nationkey
                                                  AND n_name = 'GERMANY')
ORDER  BY value DESC; 