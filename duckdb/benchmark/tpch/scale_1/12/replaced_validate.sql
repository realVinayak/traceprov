-- using default substitutions
select
    l_shipmode,
    sum(
        case
            when o_orderpriority = '1-URGENT'
            or o_orderpriority = '2-HIGH' then 1
            else 0
        end
    ) as high_line_count,
    sum(
        case
            when o_orderpriority <> '1-URGENT'
            and o_orderpriority <> '2-HIGH' then 1
            else 0
        end
    ) as low_line_count
from
    orders,
    lineitem
where
    o_orderkey = l_orderkey
    and (orders.rowid, lineitem.rowid) in (
        select
            column_1,
            column_2
        FROM
            LAYER_1_1
    )
group by
    l_shipmode
order by
    l_shipmode;