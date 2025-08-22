-- using 1755709853 as a seed to the RNG


SELECT *, mark_later(mapped_agg)
FROM (
    select
        o_orderpriority,
        count(*) as order_count,
        agg_map_parallel(o_orderkey) as mapped_agg
    from
        orders
    where
        o_orderdate >= date '1993-05-01'
        and o_orderdate < date '1993-05-01' + interval '3' month
        and exists (
            select
                *
            from
                lineitem
            where
                l_orderkey = o_orderkey
                and l_commitdate < l_receiptdate
                AND log_subquery_pk(l_orderkey, l_linenumber, o_orderkey)
        )
    group by
        o_orderpriority
) f
order by
    o_orderpriority;
