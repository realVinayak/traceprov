-- using 1755708657 as a seed to the RNG


SELECT *, mark_later(mapped_agg) FROM (
    select
        c_custkey,
        c_name,
        sum(l_extendedprice * (1 - l_discount)) as revenue,
        c_acctbal,
        n_name,
        c_address,
        c_phone,
        c_comment,
        agg_map_parallel(
            c_custkey,
            o_orderkey,
            l_orderkey,
            l_linenumber,
            n_nationkey
        ) as mapped_agg
    from
        customer,
        orders,
        lineitem,
        nation
    where
        c_custkey = o_custkey
        and l_orderkey = o_orderkey
        and o_orderdate >= date '1993-12-01'
        and o_orderdate < date '1993-12-01' + interval '3' month
        and l_returnflag = 'R'
        and c_nationkey = n_nationkey
    group by
        c_custkey,
        c_name,
        c_acctbal,
        c_phone,
        n_name,
        c_address,
        c_comment
) f
order by
    revenue desc
LIMIT 20;