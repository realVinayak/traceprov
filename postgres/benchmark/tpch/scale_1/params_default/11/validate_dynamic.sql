-- using default substitutions
-- SELECT
--     tp_table_2.ps_partkey,
--     tp_table_2.value,
--     tp_table_2.mapped_agg,
--     traceprov_log_entry (4, tp_table_2.mapped_agg) AS tp_table_3
-- FROM
--     (
--         SELECT
--             partsupp.ps_partkey,
--             sum(
--                 (
--                     partsupp.ps_supplycost * (partsupp.ps_availqty)::numeric
--                 )
--             ) AS value,
--             traceprov_agg_key_parallel_offset (
--                 3,
--                 (partsupp.ps_partkey)::bigint,
--                 (partsupp.ps_suppkey)::bigint,
--                 (supplier.s_suppkey)::bigint,
--                 (nation.n_nationkey)::bigint
--             ) AS mapped_agg
--         FROM
--             partsupp,
--             supplier,
--             nation
--         WHERE
--             (
--                 (partsupp.ps_suppkey = supplier.s_suppkey)
--                 AND (supplier.s_nationkey = nation.n_nationkey)
--                 AND (nation.n_name = 'GERMANY'::bpchar)
--             )
--         GROUP BY
--             partsupp.ps_partkey
--         HAVING
--             (
--                 sum(
--                     (
--                         partsupp.ps_supplycost * (partsupp.ps_availqty)::numeric
--                     )
--                 ) > (
--                     SELECT
--                         tp_table_1."?column?"
--                     FROM
--                         (
--                             SELECT
--                                 (
--                                     sum(
--                                         (
--                                             partsupp_1.ps_supplycost * (partsupp_1.ps_availqty)::numeric
--                                         )
--                                     ) * 0.0001000000
--                                 ) AS "?column?",
--                                 traceprov_log_entry_volatile (
--                                     2,
--                                     traceprov_agg_key_parallel_offset (
--                                         1,
--                                         (partsupp_1.ps_partkey)::bigint,
--                                         (partsupp_1.ps_suppkey)::bigint,
--                                         (supplier_1.s_suppkey)::bigint,
--                                         (nation_1.n_nationkey)::bigint
--                                     )
--                                 ) AS tp_table_0
--                             FROM
--                                 partsupp partsupp_1,
--                                 supplier supplier_1,
--                                 nation nation_1
--                             WHERE
--                                 (
--                                     (partsupp_1.ps_suppkey = supplier_1.s_suppkey)
--                                     AND (supplier_1.s_nationkey = nation_1.n_nationkey)
--                                     AND (nation_1.n_name = 'GERMANY'::bpchar)
--                                 )
--                         ) tp_table_1
--                 )
--             )
--         ORDER BY
--             (
--                 sum(
--                     (
--                         partsupp.ps_supplycost * (partsupp.ps_availqty)::numeric
--                     )
--                 )
--             ) DESC
--     ) tp_table_2;
select
    ps_partkey,
    sum(ps_supplycost * ps_availqty) as value
from
    partsupp,
    supplier,
    nation
where
    (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
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
    ps_partkey
having
    sum(ps_supplycost * ps_availqty) > (
        select
            sum(ps_supplycost * ps_availqty) * 0.0001000000
        from
            partsupp,
            supplier,
            nation
        where
            (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
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
    )
order by
    value desc;