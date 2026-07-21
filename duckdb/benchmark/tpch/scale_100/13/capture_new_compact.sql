-- using default substitutions
select *,
    traceprov_log_entry_1 (3, mapped_agg_2)
FROM (
        select c_count,
            count(*) as custdist,
            traceprov_agg_key_parallel_offset_1 (2, mapped_agg) as mapped_agg_2
        from (
                select c_custkey,
                    count(o_orderkey),
                    traceprov_agg_key_parallel_offset_2 (
                        1,
                        customer.rowid::int,
                        orders.rowid::int
                    ) as mapped_agg
                from customer
                    left outer join orders on c_custkey = o_custkey
                    and o_comment not like '%special%requests%'
                group by c_custkey
            ) as c_orders (c_custkey, c_count)
        group by c_count
        order by custdist desc,
            c_count desc
    ) F;