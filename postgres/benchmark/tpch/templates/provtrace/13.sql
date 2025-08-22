-- $ID$
-- TPC-H/TPC-R Customer Distribution Query (Q13)
-- Functional Query Definition
-- Approved February 1998
:x
:o
SELECT *, mark_later(mapped_agg_later) FROM (
    select
        c_count,
        count(*) as custdist,
        agg_from_ptr(mapped_agg) as mapped_agg_later
    from
        (
            select
                c_custkey,
                count(o_orderkey),
                agg_map_parallel(c_custkey, coalesce(o_orderkey, 0)) as mapped_agg
            from
                customer left outer join orders on
                    c_custkey = o_custkey
                    and o_comment not like '%:1%:2%'
            group by
                c_custkey
        ) as c_orders (c_custkey, c_count, mapped_agg)
    group by
        c_count
) f
order by
    custdist desc,
    c_count desc;
