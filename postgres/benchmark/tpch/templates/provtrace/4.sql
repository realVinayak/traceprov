-- $ID$
-- TPC-H/TPC-R Order Priority Checking Query (Q4)
-- Functional Query Definition
-- Approved February 1998
:x
:o
SELECT *, mark_later(mapped_agg)
FROM (
    select
        o_orderpriority,
        count(*) as order_count,
        agg_map_parallel(o_orderkey) as mapped_agg
    from
        orders
    where
        o_orderdate >= date ':1'
        and o_orderdate < date ':1' + interval '3' month
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
