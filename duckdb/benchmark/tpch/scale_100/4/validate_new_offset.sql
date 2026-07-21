-- using default substitutions
select
    o_orderpriority,
    count(*) as order_count
from
    orders
where
    orders.rowid in (
        select
            eval(column_1, column_1_1)
        from
            LAYER_2_%OUT_ID%
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
                        LAYER_1_%OUT_ID%
                )
            )
    )
group by
    o_orderpriority
order by
    o_orderpriority;