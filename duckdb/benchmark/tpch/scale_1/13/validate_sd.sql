-- using default substitutions
select c_count,
    count(*) as custdist
from (
        select c_custkey,
            count(o_orderkey)
        from customer
            left outer join orders on c_custkey = o_custkey
            and o_comment not like '%special%requests%'
        where EXISTS (
                select * from LAYER_1_SD_%OUT_ID%
                where "table" = 2 and (customer.rowid is not distinct from iid)
        )
        and EXISTS (
                select * from LAYER_1_SD_%OUT_ID%
                where "table" = 0 and (orders.rowid is not distinct from iid)
        )
        group by c_custkey
    ) as c_orders (c_custkey, c_count)
group by c_count
order by custdist desc,
    c_count desc;