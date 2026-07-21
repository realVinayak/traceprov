-- using default substitutions
select
    c_count,
    count(*) as custdist
from
    (
        select
            c_custkey,
            count(o_orderkey)
        from
            customer
            left outer join orders on c_custkey = o_custkey
            and o_comment not like '%special%requests%'
        where EXISTS (
                select *
                from LAYER_1_%OUT_ID%
                where column_1_1 is not distinct
                from customer.rowid
                    and eval(column_2, column_2_1) is not distinct
                from orders.rowid
            )
        group by
            c_custkey
    ) as c_orders (c_custkey, c_count)
group by
    c_count
order by
    custdist desc,
    c_count desc;