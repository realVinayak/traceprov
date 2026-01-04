-- using default substitutions
-- SELECT
--     tp_table_0.s_name,
--     tp_table_0.numwait,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (3, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             supplier.s_name,
--             count(*) AS numwait,
--             traceprov_agg_key_parallel_offset (
--                 2,
--                 (supplier.s_suppkey)::bigint,
--                 (l1.l_orderkey)::bigint,
--                 (l1.l_linenumber)::bigint,
--                 (orders.o_orderkey)::bigint,
--                 (nation.n_nationkey)::bigint
--             ) AS mapped_agg
--         FROM
--             supplier,
--             lineitem l1,
--             orders,
--             nation
--         WHERE
--             (
--                 (supplier.s_suppkey = l1.l_suppkey)
--                 AND (orders.o_orderkey = l1.l_orderkey)
--                 AND (orders.o_orderstatus = 'F'::bpchar)
--                 AND (l1.l_receiptdate > l1.l_commitdate)
--                 AND (
--                     EXISTS (
--                         SELECT
--                             l2.l_orderkey,
--                             l2.l_partkey,
--                             l2.l_suppkey,
--                             l2.l_linenumber,
--                             l2.l_quantity,
--                             l2.l_extendedprice,
--                             l2.l_discount,
--                             l2.l_tax,
--                             l2.l_returnflag,
--                             l2.l_linestatus,
--                             l2.l_shipdate,
--                             l2.l_commitdate,
--                             l2.l_receiptdate,
--                             l2.l_shipinstruct,
--                             l2.l_shipmode,
--                             l2.l_comment,
--                             l2.l_orderkey AS tp_l_orderkey,
--                             l2.l_linenumber AS tp_l_linenumber
--                         FROM
--                             lineitem l2
--                         WHERE
--                             (
--                                 (
--                                     (l2.l_orderkey = l1.l_orderkey)
--                                     AND (l2.l_suppkey <> l1.l_suppkey)
--                                 )
--                                 AND CASE
--                                     WHEN (
--                                         (l2.l_orderkey = l1.l_orderkey)
--                                         AND (l2.l_suppkey <> l1.l_suppkey)
--                                     ) THEN traceprov_log_entry (
--                                         1,
--                                         (l1.l_orderkey)::bigint,
--                                         (l1.l_linenumber)::bigint,
--                                         (l1.l_orderkey)::bigint,
--                                         (l1.l_linenumber)::bigint,
--                                         (l2.l_orderkey)::bigint,
--                                         (l2.l_linenumber)::bigint
--                                     )
--                                     ELSE false
--                                 END
--                             )
--                     )
--                 )
--                 AND (
--                     NOT (
--                         EXISTS (
--                             SELECT
--                                 l3.l_orderkey,
--                                 l3.l_partkey,
--                                 l3.l_suppkey,
--                                 l3.l_linenumber,
--                                 l3.l_quantity,
--                                 l3.l_extendedprice,
--                                 l3.l_discount,
--                                 l3.l_tax,
--                                 l3.l_returnflag,
--                                 l3.l_linestatus,
--                                 l3.l_shipdate,
--                                 l3.l_commitdate,
--                                 l3.l_receiptdate,
--                                 l3.l_shipinstruct,
--                                 l3.l_shipmode,
--                                 l3.l_comment
--                             FROM
--                                 lineitem l3
--                             WHERE
--                                 (
--                                     (l3.l_orderkey = l1.l_orderkey)
--                                     AND (l3.l_suppkey <> l1.l_suppkey)
--                                     AND (l3.l_receiptdate > l3.l_commitdate)
--                                 )
--                         )
--                     )
--                 )
--                 AND (supplier.s_nationkey = nation.n_nationkey)
--                 AND (nation.n_name = 'SAUDI ARABIA'::bpchar)
--             )
--         GROUP BY
--             supplier.s_name
--         ORDER BY
--             (count(*)) DESC,
--             supplier.s_name
--         LIMIT
--             100
--     ) tp_table_0;
select
    s_name,
    count(*) as numwait
from
    supplier,
    lineitem l1,
    orders,
    nation
where
    exists (
        select
            *
        from
            lineitem l2
        where
            l2.l_orderkey = l1.l_orderkey
            and l2.l_suppkey <> l1.l_suppkey
            and (l2.l_orderkey, l2.l_linenumber) in (
                select
                    col4,
                    col5
                from
                    traceprov_perform_derivation (1, false) as (
                        col0 bigint,
                        col1 bigint,
                        col2 bigint,
                        col3 bigint,
                        col4 bigint,
                        col5 bigint
                    )
            )
    )
    and not exists (
        select
            *
        from
            lineitem l3
        where
            l3.l_orderkey = l1.l_orderkey
            and l3.l_suppkey <> l1.l_suppkey
            and l3.l_receiptdate > l3.l_commitdate
    )
    and (
        s_suppkey,
        l_orderkey,
        l_linenumber,
        o_orderkey,
        n_nationkey
    ) in (
        select
            col1,
            col2,
            col3,
            col4,
            col5
        from
            traceprov_perform_derivation (3, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint,
                col4 bigint,
                col5 bigint
            )
    )
group by
    s_name
order by
    numwait desc,
    s_name
LIMIT
    100;