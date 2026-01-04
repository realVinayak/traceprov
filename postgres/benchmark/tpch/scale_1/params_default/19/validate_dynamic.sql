-- using default substitutions
-- SELECT
--     tp_table_0.revenue,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             sum(
--                 (
--                     lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                 )
--             ) AS revenue,
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
--                 (
--                     (part.p_partkey = lineitem.l_partkey)
--                     AND (part.p_brand = 'Brand#12'::bpchar)
--                     AND (
--                         part.p_container = ANY (
--                             ARRAY[
--                                 'SM CASE'::bpchar,
--                                 'SM BOX'::bpchar,
--                                 'SM PACK'::bpchar,
--                                 'SM PKG'::bpchar
--                             ]
--                         )
--                     )
--                     AND (lineitem.l_quantity >= (1)::numeric)
--                     AND (lineitem.l_quantity <= ((1 + 10))::numeric)
--                     AND (
--                         (part.p_size >= 1)
--                         AND (part.p_size <= 5)
--                     )
--                     AND (
--                         lineitem.l_shipmode = ANY (ARRAY['AIR'::bpchar, 'AIR REG'::bpchar])
--                     )
--                     AND (
--                         lineitem.l_shipinstruct = 'DELIVER IN PERSON'::bpchar
--                     )
--                 )
--                 OR (
--                     (part.p_partkey = lineitem.l_partkey)
--                     AND (part.p_brand = 'Brand#23'::bpchar)
--                     AND (
--                         part.p_container = ANY (
--                             ARRAY[
--                                 'MED BAG'::bpchar,
--                                 'MED BOX'::bpchar,
--                                 'MED PKG'::bpchar,
--                                 'MED PACK'::bpchar
--                             ]
--                         )
--                     )
--                     AND (lineitem.l_quantity >= (10)::numeric)
--                     AND (lineitem.l_quantity <= ((10 + 10))::numeric)
--                     AND (
--                         (part.p_size >= 1)
--                         AND (part.p_size <= 10)
--                     )
--                     AND (
--                         lineitem.l_shipmode = ANY (ARRAY['AIR'::bpchar, 'AIR REG'::bpchar])
--                     )
--                     AND (
--                         lineitem.l_shipinstruct = 'DELIVER IN PERSON'::bpchar
--                     )
--                 )
--                 OR (
--                     (part.p_partkey = lineitem.l_partkey)
--                     AND (part.p_brand = 'Brand#34'::bpchar)
--                     AND (
--                         part.p_container = ANY (
--                             ARRAY[
--                                 'LG CASE'::bpchar,
--                                 'LG BOX'::bpchar,
--                                 'LG PACK'::bpchar,
--                                 'LG PKG'::bpchar
--                             ]
--                         )
--                     )
--                     AND (lineitem.l_quantity >= (20)::numeric)
--                     AND (lineitem.l_quantity <= ((20 + 10))::numeric)
--                     AND (
--                         (part.p_size >= 1)
--                         AND (part.p_size <= 15)
--                     )
--                     AND (
--                         lineitem.l_shipmode = ANY (ARRAY['AIR'::bpchar, 'AIR REG'::bpchar])
--                     )
--                     AND (
--                         lineitem.l_shipinstruct = 'DELIVER IN PERSON'::bpchar
--                     )
--                 )
--             )
--     ) tp_table_0;
select
    sum(l_extendedprice * (1 - l_discount)) as revenue
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