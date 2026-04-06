-- using default substitutions
select
    o_orderpriority,
    count(*) as order_count
from
    orders
where
    orders.rowid in (
        select
            column_1
        from
            LAYER_2
    )
    and exists (
        select
            *
        from
            lineitem
        where
            l_orderkey = o_orderkey
            and (
                lineitem.rowid in (
                    select
                        column_1_1
                    from
                        LAYER_1
                )
            )
    )
group by
    o_orderpriority
order by
    o_orderpriority;