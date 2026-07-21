-- using default substitutions
select c_count,
    count(*) as custdist
from (
        select c_custkey,
            count(o_orderkey)
        from customer
            left outer join orders on c_custkey = o_custkey
            and o_comment not like '%special%requests%'
        where exists (
                select *
                from traceprov_lineage_1
                where column_1_1 is not distinct
                from c_custkey
                    and column_2 is not distinct
                from o_orderkey
            )
        group by c_custkey
    ) as c_orders (c_custkey, c_count)
group by c_count
order by custdist desc,
    c_count desc;