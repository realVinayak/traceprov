-- using 1755703766 as a seed to the RNG


SELECT *, mark_later(mapped_agg_group_1), mark_later(mapped_agg_group_2) FROM (
    select
        ps_partkey,
        sum(ps_supplycost * ps_availqty) as value,
        traceprov_agg_key_parallel(1, ps_suppkey, ps_partkey, s_suppkey, n_nationkey) as mapped_agg_group_1
    from
        partsupp,
        supplier,
        nation
    where
        ps_suppkey = s_suppkey
        and s_nationkey = n_nationkey
        and n_name = 'KENYA'
    group by
        ps_partkey
) a
JOIN (
    select
        sum(ps_supplycost * ps_availqty) * 0.0001000000 as computed_value,
        traceprov_agg_key_parallel(4, ps_suppkey, ps_partkey, s_suppkey, n_nationkey) as mapped_agg_group_2
    from
        partsupp,
        supplier,
        nation
    where
        ps_suppkey = s_suppkey
        and s_nationkey = n_nationkey
        and n_name = 'KENYA'
) b
ON a.value > b.computed_value
order by value desc;


