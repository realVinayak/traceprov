-- using 1755708649 as a seed to the RNG


SELECT *, mark_later(mapped_agg_later) FROM (
    select
        c_count,
        count(*) as custdist,
        traceprov_agg_from_ptr(1, 5, mapped_agg) as mapped_agg_later
    from
        (
            select
                c_custkey,
                count(o_orderkey),
                traceprov_agg_key_parallel(1, c_custkey, coalesce(o_orderkey, 0)) as mapped_agg
            from
                customer left outer join orders on
                    c_custkey = o_custkey
                    and o_comment not like '%pending%packages%'
            group by
                c_custkey
        ) as c_orders (c_custkey, c_count, mapped_agg)
    group by
        c_count
) f
order by
    custdist desc,
    c_count desc;
