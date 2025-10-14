-- using 1755709859 as a seed to the RNG


SELECT *, mark_later(mapped_agg) FROM (
    select
        sum(l_extendedprice * l_discount) as revenue,
        traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
    from
        lineitem
    where
        l_shipdate >= date '1995-01-01'
        and l_shipdate < date '1995-01-01' + interval '1' year
        and l_discount between 0.04 - 0.01 and 0.04 + 0.01
        and l_quantity < 25
) f;
