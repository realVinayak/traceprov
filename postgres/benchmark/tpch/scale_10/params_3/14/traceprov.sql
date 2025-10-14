-- using 1755709849 as a seed to the RNG


SELECT *, mark_later(mapped_agg) FROM (
    select
        100.00 * sum(case
            when p_type like 'PROMO%'
                then l_extendedprice * (1 - l_discount)
            else 0
        end) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue,
        traceprov_agg_key_parallel(1, l_orderkey, l_linenumber, p_partkey) as mapped_agg
    from
        lineitem,
        part
    where
        l_partkey = p_partkey
        and l_shipdate >= date '1993-12-01'
        and l_shipdate < date '1993-12-01' + interval '1' month
) f;

