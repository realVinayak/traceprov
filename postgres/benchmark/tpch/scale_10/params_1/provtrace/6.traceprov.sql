-- using 1755709829 as a seed to the RNG


SELECT *, mark_later(mapped_agg) FROM (
    select
        sum(l_extendedprice * l_discount) as revenue,
        agg_map_parallel(l_orderkey, l_linenumber) as mapped_agg
    from
        lineitem
    where
        l_shipdate >= date '1993-01-01'
        and l_shipdate < date '1993-01-01' + interval '1' year
        and l_discount between 0.09 - 0.01 and 0.09 + 0.01
        and l_quantity < 25
) f;
