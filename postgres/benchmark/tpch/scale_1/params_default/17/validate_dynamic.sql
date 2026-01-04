-- using default substitutions
-- SELECT
--     tp_table_2.avg_yearly,
--     tp_table_2.mapped_agg,
--     traceprov_log_entry (4, tp_table_2.mapped_agg) AS tp_table_3
-- FROM
--     (
--         SELECT
--             (sum(lineitem.l_extendedprice) / 7.0) AS avg_yearly,
--             traceprov_agg_key_parallel_offset (
--                 3,
--                 (lineitem.l_orderkey)::bigint,
--                 (lineitem.l_linenumber)::bigint,
--                 (part.p_partkey)::bigint
--             ) AS mapped_agg
--         FROM
--             lineitem,
--             part
--         WHERE
--             (
--                 (part.p_partkey = lineitem.l_partkey)
--                 AND (part.p_brand = 'Brand#23'::bpchar)
--                 AND (part.p_container = 'MED BOX'::bpchar)
--                 AND (
--                     lineitem.l_quantity < (
--                         SELECT
--                             tp_table_1."?column?"
--                         FROM
--                             (
--                                 SELECT
--                                     (0.2 * avg(lineitem_1.l_quantity)) AS "?column?",
--                                     traceprov_log_entry_volatile (
--                                         2,
--                                         (part.p_partkey)::bigint,
--                                         traceprov_agg_key_parallel_offset (
--                                             1,
--                                             (lineitem_1.l_orderkey)::bigint,
--                                             (lineitem_1.l_linenumber)::bigint
--                                         )
--                                     ) AS tp_table_0
--                                 FROM
--                                     lineitem lineitem_1
--                                 WHERE
--                                     (lineitem_1.l_partkey = part.p_partkey)
--                             ) tp_table_1
--                     )
--                 )
--             )
--     ) tp_table_2;
select
    sum(l_extendedprice) / 7.0 as avg_yearly
from
    lineitem,
    part
where
    (l_orderkey, l_linenumber, p_partkey) in (
        select
            col1,
            col2,
            col3
        from
            traceprov_perform_derivation (4, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint
            )
    )
    and l_quantity < (
        select
            0.2 * avg(l_quantity)
        from
            lineitem
        where
            (p_partkey, l_orderkey, l_linenumber) in (
                select
                    col0,
                    col2,
                    col3
                from
                    traceprov_perform_derivation (2, false) as (
                        col0 bigint,
                        col1 bigint,
                        col2 bigint,
                        col3 bigint
                    )
            )
    );