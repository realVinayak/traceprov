-- using default substitutions
-- SELECT
--     tp_table_6.s_name,
--     tp_table_6.s_address,
--     tp_table_6.tp_s_suppkey,
--     tp_table_6.tp_n_nationkey,
--     traceprov_log_entry (
--         5,
--         (tp_table_6.tp_s_suppkey)::bigint,
--         (tp_table_6.tp_n_nationkey)::bigint
--     ) AS tp_table_7
-- FROM
--     (
--         SELECT
--             supplier.s_name,
--             supplier.s_address,
--             supplier.s_suppkey AS tp_s_suppkey,
--             nation.n_nationkey AS tp_n_nationkey
--         FROM
--             supplier,
--             nation
--         WHERE
--             (
--                 (
--                     supplier.s_suppkey IN (
--                         SELECT
--                             tp_table_5.ps_suppkey
--                         FROM
--                             (
--                                 SELECT
--                                     partsupp.ps_suppkey,
--                                     traceprov_log_entry_volatile (
--                                         4,
--                                         (partsupp.ps_partkey)::bigint,
--                                         (partsupp.ps_suppkey)::bigint
--                                     ) AS tp_table_4
--                                 FROM
--                                     partsupp
--                                 WHERE
--                                     (
--                                         (
--                                             partsupp.ps_partkey IN (
--                                                 SELECT
--                                                     tp_table_1.p_partkey
--                                                 FROM
--                                                     (
--                                                         SELECT
--                                                             part.p_partkey,
--                                                             traceprov_log_entry_volatile (1, (part.p_partkey)::bigint) AS tp_table_0
--                                                         FROM
--                                                             part
--                                                         WHERE
--                                                             ((part.p_name)::text ~~ 'forest%'::text)
--                                                     ) tp_table_1
--                                             )
--                                         )
--                                         AND (
--                                             (partsupp.ps_availqty)::numeric > (
--                                                 SELECT
--                                                     tp_table_3."?column?"
--                                                 FROM
--                                                     (
--                                                         SELECT
--                                                             (0.5 * sum(lineitem.l_quantity)) AS "?column?",
--                                                             traceprov_log_entry_volatile (
--                                                                 3,
--                                                                 (partsupp.ps_partkey)::bigint,
--                                                                 (partsupp.ps_suppkey)::bigint,
--                                                                 (partsupp.ps_partkey)::bigint,
--                                                                 (partsupp.ps_suppkey)::bigint,
--                                                                 traceprov_agg_key_parallel_offset (
--                                                                     2,
--                                                                     (lineitem.l_orderkey)::bigint,
--                                                                     (lineitem.l_linenumber)::bigint
--                                                                 )
--                                                             ) AS tp_table_2
--                                                         FROM
--                                                             lineitem
--                                                         WHERE
--                                                             (
--                                                                 (lineitem.l_partkey = partsupp.ps_partkey)
--                                                                 AND (lineitem.l_suppkey = partsupp.ps_suppkey)
--                                                                 AND (lineitem.l_shipdate >= '1994-01-01'::date)
--                                                                 AND (
--                                                                     lineitem.l_shipdate < ('1994-01-01'::date + '1 year'::interval year)
--                                                                 )
--                                                             )
--                                                     ) tp_table_3
--                                             )
--                                         )
--                                     )
--                             ) tp_table_5
--                     )
--                 )
--                 AND (supplier.s_nationkey = nation.n_nationkey)
--                 AND (nation.n_name = 'CANADA'::bpchar)
--             )
--         ORDER BY
--             supplier.s_name
--     ) tp_table_6;
select
    s_name,
    s_address
from
    supplier,
    nation
where
    s_suppkey in (
        select
            ps_suppkey
        from
            partsupp
        where
            ps_partkey in (
                select
                    p_partkey
                from
                    part
                where
                    p_partkey in (
                        select
                            col0
                        from
                            traceprov_perform_derivation (1, false) as (col0 bigint)
                    )
            )
            and ps_availqty > (
                select
                    0.5 * sum(l_quantity)
                from
                    lineitem
                where
                    l_partkey = ps_partkey
                    and l_suppkey = ps_suppkey
                    and (l_orderkey, l_linenumber) in (
                        select
                            col5,
                            col6
                        from
                            traceprov_perform_derivation (3, false) as (
                                col0 bigint,
                                col1 bigint,
                                col2 bigint,
                                col3 bigint,
                                col4 bigint,
                                col5 bigint,
                                col6 bigint
                            )
                    )
            )
            and (ps_partkey, ps_suppkey) in (
                select
                    col0,
                    col1
                from
                    traceprov_perform_derivation (4, false) as (col0 bigint, col1 bigint)
            )
    )
    and (s_suppkey, n_nationkey) in (
        select
            col0,
            col1
        from
            traceprov_perform_derivation (5, false) as (col0 bigint, col1 bigint)
    )
order by
    s_name;