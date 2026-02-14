-- using default substitutions
-- SELECT
--     tp_table_0.p_brand,
--     tp_table_0.p_type,
--     tp_table_0.p_size,
--     tp_table_0.supplier_cnt,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             part.p_brand,
--             part.p_type,
--             part.p_size,
--             count(DISTINCT partsupp.ps_suppkey) AS supplier_cnt,
--             traceprov_agg_key_parallel_offset (
--                 1,
--                 (partsupp.ps_partkey)::bigint,
--                 (partsupp.ps_suppkey)::bigint,
--                 (part.p_partkey)::bigint
--             ) AS mapped_agg
--         FROM
--             partsupp,
--             part
--         WHERE
--             (
--                 (part.p_partkey = partsupp.ps_partkey)
--                 AND (part.p_brand <> 'Brand#45'::bpchar)
--                 AND ((part.p_type)::text !~~ 'MEDIUM POLISHED%'::text)
--                 AND (
--                     part.p_size = ANY (ARRAY[49, 14, 23, 45, 19, 3, 36, 9])
--                 )
--                 AND (
--                     NOT (
--                         partsupp.ps_suppkey IN (
--                             SELECT
--                                 supplier.s_suppkey
--                             FROM
--                                 supplier
--                             WHERE
--                                 (
--                                     (supplier.s_comment)::text ~~ '%Customer%Complaints%'::text
--                                 )
--                         )
--                     )
--                 )
--             )
--         GROUP BY
--             part.p_brand,
--             part.p_type,
--             part.p_size
--         ORDER BY
--             (count(DISTINCT partsupp.ps_suppkey)) DESC,
--             part.p_brand,
--             part.p_type,
--             part.p_size
--     ) tp_table_0;
select
    p_brand,
    p_type,
    p_size,
    count(distinct ps_suppkey) as supplier_cnt
from
    partsupp,
    part
where
    (ps_partkey, ps_suppkey, p_partkey) in (
        select
            col1,
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
group by
    p_brand,
    p_type,
    p_size
order by
    supplier_cnt desc,
    p_brand,
    p_type,
    p_size;