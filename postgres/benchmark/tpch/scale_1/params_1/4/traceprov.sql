-- using 1755693633 as a seed to the RNG


SELECT *, mark_later(mapped_agg)
FROM (
    select
        o_orderpriority,
        count(*) as order_count,
        traceprov_agg_key_parallel(1, o_orderkey) as mapped_agg
    from
        orders
    where
        o_orderdate >= date '1995-11-01'
        and o_orderdate < date '1995-11-01' + interval '3' month
        and exists (
            select
                *
            from
                lineitem
            where
                l_orderkey = o_orderkey
                and l_commitdate < l_receiptdate
                AND traceprov_log_subquery_pk(4, l_orderkey, l_linenumber, o_orderkey)
        )
    group by
        o_orderpriority
) f
order by
    o_orderpriority;
