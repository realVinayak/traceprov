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
        where
            (customer.rowid, coalesce(orders.rowid, 0)) in (
                select
                    opid_12_customer,
                    opid_10_orders
                from
                    LAYER_1
            )
        group by
            c_custkey
    ) as c_orders (c_custkey, c_count)
group by
    c_count
order by
    custdist desc,
    c_count desc;
