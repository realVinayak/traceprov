-- SELECT
--     tp_table_0.n_name,
--     tp_table_0.revenue,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             nation.n_name,
--             sum(
--                 (
--                     lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                 )
--             ) AS revenue,
--             traceprov_agg_key_parallel_offset (
--                 1,
--                 (customer.c_custkey)::bigint,
--                 (orders.o_orderkey)::bigint,
--                 (lineitem.l_orderkey)::bigint,
--                 (lineitem.l_linenumber)::bigint,
--                 (supplier.s_suppkey)::bigint,
--                 (nation.n_nationkey)::bigint,
--                 (region.r_regionkey)::bigint
--             ) AS mapped_agg
--         FROM
--             customer,
--             orders,
--             lineitem,
--             supplier,
--             nation,
--             region
--         WHERE
--             (
--                 (customer.c_custkey = orders.o_custkey)
--                 AND (lineitem.l_orderkey = orders.o_orderkey)
--                 AND (lineitem.l_suppkey = supplier.s_suppkey)
--                 AND (customer.c_nationkey = supplier.s_nationkey)
--                 AND (supplier.s_nationkey = nation.n_nationkey)
--                 AND (nation.n_regionkey = region.r_regionkey)
--                 AND (region.r_name = 'ASIA'::bpchar)
--                 AND (orders.o_orderdate >= '1994-01-01'::date)
--                 AND (
--                     orders.o_orderdate < ('1994-01-01'::date + '1 year'::interval year)
--                 )
--             )
--         GROUP BY
--             nation.n_name
--         ORDER BY
--             (
--                 sum(
--                     (
--                         lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                     )
--                 )
--             ) DESC
--     ) tp_table_0;
select
    n_name,
    sum(l_extendedprice * (1 - l_discount)) as revenue
from
    customer,
    orders,
    lineitem,
    supplier,
    nation,
    region
where
    (
        c_custkey,
        o_orderkey,
        l_orderkey,
        l_linenumber,
        s_suppkey,
        n_nationkey,
        r_regionkey
    ) in (
        select
            col1,
            col2,
            col3,
            col4,
            col5,
            col6,
            col7
        from
            traceprov_perform_derivation (2, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint,
                col4 bigint,
                col5 bigint,
                col6 bigint,
                col7 bigint
            )
    )
group by
    n_name
order by
    revenue desc;