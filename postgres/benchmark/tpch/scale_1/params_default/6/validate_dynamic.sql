-- using default substitutions
-- SELECT
--     tp_table_0.revenue,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             sum((lineitem.l_extendedprice * lineitem.l_discount)) AS revenue,
--             traceprov_agg_key_parallel_offset (
--                 1,
--                 (lineitem.l_orderkey)::bigint,
--                 (lineitem.l_linenumber)::bigint
--             ) AS mapped_agg
--         FROM
--             lineitem
--         WHERE
--             (
--                 (lineitem.l_shipdate >= '1994-01-01'::date)
--                 AND (
--                     lineitem.l_shipdate < ('1994-01-01'::date + '1 year'::interval year)
--                 )
--                 AND (
--                     (lineitem.l_discount >= (0.06 - 0.01))
--                     AND (lineitem.l_discount <= (0.06 + 0.01))
--                 )
--                 AND (lineitem.l_quantity < (24)::numeric)
--             )
--     ) tp_table_0;
select
    sum(l_extendedprice * l_discount) as revenue
from
    lineitem
where
    (l_orderkey, l_linenumber) in (
        select
            col1,
            col2
        from
            traceprov_perform_derivation (2, false) as (col0 bigint, col1 bigint, col2 bigint)
    );