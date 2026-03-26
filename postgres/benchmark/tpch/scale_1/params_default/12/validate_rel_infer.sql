select l_shipmode,
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
from orders,
    lineitem
where (o_orderkey, l_orderkey, l_linenumber) in (
        select col_1,
            col_2,
            col_3
        from traceprov_relation_infer_1_mat
    )
group by l_shipmode
order by l_shipmode;