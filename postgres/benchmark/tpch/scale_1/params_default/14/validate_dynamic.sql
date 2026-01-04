-- -- using default substitutions
-- SELECT
--     tp_table_0.promo_revenue,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             (
--                 (
--                     100.00 * sum(
--                         CASE
--                             WHEN ((part.p_type)::text ~~ 'PROMO%'::text) THEN (
--                                 lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                             )
--                             ELSE (0)::numeric
--                         END
--                     )
--                 ) / sum(
--                     (
--                         lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                     )
--                 )
--             ) AS promo_revenue,
--             traceprov_agg_key_parallel_offset (
--                 1,
--                 (lineitem.l_orderkey)::bigint,
--                 (lineitem.l_linenumber)::bigint,
--                 (part.p_partkey)::bigint
--             ) AS mapped_agg
--         FROM
--             lineitem,
--             part
--         WHERE
--             (
--                 (lineitem.l_partkey = part.p_partkey)
--                 AND (lineitem.l_shipdate >= '1995-09-01'::date)
--                 AND (
--                     lineitem.l_shipdate < ('1995-09-01'::date + '1 mon'::interval month)
--                 )
--             )
--     ) tp_table_0;
select
    100.00 * sum(
        case
            when p_type like 'PROMO%' then l_extendedprice * (1 - l_discount)
            else 0
        end
    ) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue
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
            traceprov_perform_derivation (2, false) as (
                col0 bigint,
                col1 bigint,
                col2 bigint,
                col3 bigint
            )
    );