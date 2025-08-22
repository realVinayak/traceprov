-- using 1755693633 as a seed to the RNG

SELECT *, mark_later(mapped_agg_group_1), mark_later(mapped_agg_group_2) FROM (
    SELECT ps_partkey,
       SUM(ps_supplycost * ps_availqty) AS value,
       agg_map_parallel_second(ps_suppkey, ps_partkey, s_suppkey, n_nationkey) as mapped_agg_group_1
    FROM
        partsupp,
        supplier,
        nation
    WHERE  ps_suppkey = s_suppkey
        AND s_nationkey = n_nationkey
        AND n_name = 'GERMANY'
    GROUP  BY ps_partkey
) a
JOIN (
    SELECT
       SUM(ps_supplycost * ps_availqty) * 0.0001000000 as computed_value,
       agg_map_parallel(ps_suppkey, ps_partkey, s_suppkey, n_nationkey) as mapped_agg_group_2
    FROM 
        partsupp,
        supplier,
        nation
    WHERE  ps_suppkey = s_suppkey
            AND s_nationkey = n_nationkey
            AND n_name = 'GERMANY'
    ) b
on a.value > b.computed_value
ORDER  BY value DESC; 


