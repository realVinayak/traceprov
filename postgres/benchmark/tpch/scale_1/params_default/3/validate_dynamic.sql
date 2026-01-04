-- using default substitutions
-- SELECT
--     tp_table_0.l_orderkey,
--     tp_table_0.revenue,
--     tp_table_0.o_orderdate,
--     tp_table_0.o_shippriority,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             lineitem.l_orderkey,
--             sum(
--                 (
--                     lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                 )
--             ) AS revenue,
--             orders.o_orderdate,
--             orders.o_shippriority,
--             traceprov_agg_key_parallel_offset (
--                 1,
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
--                 (customer.c_mktsegment = 'BUILDING'::bpchar)
--                 AND (customer.c_custkey = orders.o_custkey)
--                 AND (lineitem.l_orderkey = orders.o_orderkey)
--                 AND (orders.o_orderdate < '1995-03-15'::date)
--                 AND (lineitem.l_shipdate > '1995-03-15'::date)
--             )
--         GROUP BY
--             lineitem.l_orderkey,
--             orders.o_orderdate,
--             orders.o_shippriority
--         ORDER BY
--             (
--                 sum(
--                     (
--                         lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                     )
--                 )
--             ) DESC,
--             orders.o_orderdate
--         LIMIT
--             10
--     ) tp_table_0;
select
    l_orderkey,
    sum(l_extendedprice * (1 - l_discount)) as revenue,
    o_orderdate,
    o_shippriority
from
    customer,
    orders,
    lineitem
where
    (c_custkey, o_orderkey, l_orderkey, l_linenumber) in (
        select
            col1,
            col2,
            col3,
            col4
        from
            traceprov_perform_derivation (2, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint,
                col4 bigint
            )
    )
group by
    l_orderkey,
    o_orderdate,
    o_shippriority
order by
    revenue desc,
    o_orderdate;