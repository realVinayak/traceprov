-- using default substitutions
-- SELECT
--     tp_table_2.c_name,
--     tp_table_2.c_custkey,
--     tp_table_2.o_orderkey,
--     tp_table_2.o_orderdate,
--     tp_table_2.o_totalprice,
--     tp_table_2.sum,
--     tp_table_2.mapped_agg,
--     traceprov_log_entry (4, tp_table_2.mapped_agg) AS tp_table_3
-- FROM
--     (
--         SELECT
--             customer.c_name,
--             customer.c_custkey,
--             orders.o_orderkey,
--             orders.o_orderdate,
--             orders.o_totalprice,
--             sum(lineitem.l_quantity) AS sum,
--             traceprov_agg_key_parallel_offset (
--                 3,
--                 (customer.c_custkey)::bigint,
--                 (orders.o_orderkey)::bigint,
--                 (lineitem.l_orderkey)::bigint,
--                 (lineitem.l_linenumber)::bigint
--             ) AS mapped_agg
--         FROM
--             customer,
--             orders,
--             lineitem
--         WHERE
--             (
--                 (
--                     orders.o_orderkey IN (
--                         SELECT
--                             tp_table_1.l_orderkey
--                         FROM
--                             (
--                                 SELECT
--                                     lineitem_1.l_orderkey,
--                                     traceprov_log_entry_volatile (
--                                         2,
--                                         traceprov_agg_key_parallel_offset (
--                                             1,
--                                             (lineitem_1.l_orderkey)::bigint,
--                                             (lineitem_1.l_linenumber)::bigint
--                                         )
--                                     ) AS tp_table_0
--                                 FROM
--                                     lineitem lineitem_1
--                                 GROUP BY
--                                     lineitem_1.l_orderkey
--                                 HAVING
--                                     (sum(lineitem_1.l_quantity) > (300)::numeric)
--                             ) tp_table_1
--                     )
--                 )
--                 AND (customer.c_custkey = orders.o_custkey)
--                 AND (orders.o_orderkey = lineitem.l_orderkey)
--             )
--         GROUP BY
--             customer.c_name,
--             customer.c_custkey,
--             orders.o_orderkey,
--             orders.o_orderdate,
--             orders.o_totalprice
--         ORDER BY
--             orders.o_totalprice DESC,
--             orders.o_orderdate
--         LIMIT
--             100
--     ) tp_table_2;
select
    c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice,
    sum(l_quantity)
from
    customer,
    orders,
    lineitem
where
    o_orderkey in (
        select
            l_orderkey
        from
            lineitem
        where
            (l_orderkey, l_linenumber) in (
                select
                    col1,
                    col2
                from
                    traceprov_perform_derivation (2, false) as (col0 bigint, col1 bigint, col2 bigint)
            )
        group by
            l_orderkey
        having
            sum(l_quantity) > 300
    )
    and (c_custkey, o_orderkey, l_orderkey, l_linenumber) in (
        select
            col1,
            col2,
            col3,
            col4
        from
            traceprov_perform_derivation (4, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint,
                col4 bigint
            )
    )
group by
    c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice
order by
    o_totalprice desc,
    o_orderdate
LIMIT
    100;