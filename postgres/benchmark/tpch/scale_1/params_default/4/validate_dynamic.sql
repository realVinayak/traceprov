-- using default substitutions
-- SELECT
--     tp_table_0.o_orderpriority,
--     tp_table_0.order_count,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (3, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             orders.o_orderpriority,
--             count(*) AS order_count,
--             traceprov_agg_key_parallel_offset (2, (orders.o_orderkey)::bigint) AS mapped_agg
--         FROM
--             orders
--         WHERE
--             (
--                 (orders.o_orderdate >= '1993-07-01'::date)
--                 AND (
--                     orders.o_orderdate < ('1993-07-01'::date + '3 mons'::interval month)
--                 )
--                 AND (
--                     EXISTS (
--                         SELECT
--                             lineitem.l_orderkey,
--                             lineitem.l_partkey,
--                             lineitem.l_suppkey,
--                             lineitem.l_linenumber,
--                             lineitem.l_quantity,
--                             lineitem.l_extendedprice,
--                             lineitem.l_discount,
--                             lineitem.l_tax,
--                             lineitem.l_returnflag,
--                             lineitem.l_linestatus,
--                             lineitem.l_shipdate,
--                             lineitem.l_commitdate,
--                             lineitem.l_receiptdate,
--                             lineitem.l_shipinstruct,
--                             lineitem.l_shipmode,
--                             lineitem.l_comment,
--                             lineitem.l_orderkey AS tp_l_orderkey,
--                             lineitem.l_linenumber AS tp_l_linenumber
--                         FROM
--                             lineitem
--                         WHERE
--                             (
--                                 (
--                                     (lineitem.l_orderkey = orders.o_orderkey)
--                                     AND (lineitem.l_commitdate < lineitem.l_receiptdate)
--                                 )
--                                 AND CASE
--                                     WHEN (
--                                         (lineitem.l_orderkey = orders.o_orderkey)
--                                         AND (lineitem.l_commitdate < lineitem.l_receiptdate)
--                                     ) THEN traceprov_log_entry (
--                                         1,
--                                         (orders.o_orderkey)::bigint,
--                                         (lineitem.l_orderkey)::bigint,
--                                         (lineitem.l_linenumber)::bigint
--                                     )
--                                     ELSE false
--                                 END
--                             )
--                     )
--                 )
--             )
--         GROUP BY
--             orders.o_orderpriority
--         ORDER BY
--             orders.o_orderpriority
--     ) tp_table_0;
select
    o_orderpriority,
    count(*) as order_count
from
    orders
where
    o_orderkey in (
        select
            col1
        from
            traceprov_perform_derivation (3, false) as (col0 bigint, col1 bigint)
    )
    and exists (
        select
            *
        from
            lineitem
        where
            l_orderkey = o_orderkey
            and (l_orderkey, l_linenumber) in (
                select
                    col1,
                    col2
                from
                    traceprov_perform_derivation (1, false) as (col0 bigint, col1 bigint, col2 bigint)
            )
    )
group by
    o_orderpriority
order by
    o_orderpriority;