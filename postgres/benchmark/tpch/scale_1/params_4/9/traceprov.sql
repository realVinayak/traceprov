-- using 1755708649 as a seed to the RNG


SELECT *, mark_later(mapped_agg) FROM (
    select
        nation,
        o_year,
        sum(amount) as sum_profit,
        traceprov_agg_key_parallel(
                1,
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                ps_partkey,
                ps_suppkey,
                o_orderkey,
                n_nationkey
        ) as mapped_agg
    from
        (
            select
                n_name as nation,
                extract(year from o_orderdate) as o_year,
                l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount,
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                ps_partkey,
                ps_suppkey,
                o_orderkey,
                n_nationkey
            from
                part,
                supplier,
                lineitem,
                partsupp,
                orders,
                nation
            where
                s_suppkey = l_suppkey
                and ps_suppkey = l_suppkey
                and ps_partkey = l_partkey
                and p_partkey = l_partkey
                and o_orderkey = l_orderkey
                and s_nationkey = n_nationkey
                and p_name like '%midnight%'
        ) as profit
    group by
        nation,
        o_year
) f
order by
    nation,
    o_year desc;
