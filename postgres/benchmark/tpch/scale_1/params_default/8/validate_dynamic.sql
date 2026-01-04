-- using default substitutions
-- SELECT
--     tp_table_0.o_year,
--     tp_table_0.mkt_share,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (2, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             all_nations.o_year,
--             (
--                 sum(
--                     CASE
--                         WHEN (all_nations.nation = 'BRAZIL'::bpchar) THEN all_nations.volume
--                         ELSE (0)::numeric
--                     END
--                 ) / sum(all_nations.volume)
--             ) AS mkt_share,
--             traceprov_agg_key_parallel_offset (
--                 1,
--                 (all_nations.tp_p_partkey)::bigint,
--                 (all_nations.tp_s_suppkey)::bigint,
--                 (all_nations.tp_l_orderkey)::bigint,
--                 (all_nations.tp_l_linenumber)::bigint,
--                 (all_nations.tp_o_orderkey)::bigint,
--                 (all_nations.tp_c_custkey)::bigint,
--                 (all_nations.tp_n_nationkey)::bigint,
--                 (all_nations.tp_n_nationkey_1)::bigint,
--                 (all_nations.tp_r_regionkey)::bigint
--             ) AS mapped_agg
--         FROM
--             (
--                 SELECT
--                     EXTRACT(
--                         year
--                         FROM
--                             orders.o_orderdate
--                     ) AS o_year,
--                     (
--                         lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
--                     ) AS volume,
--                     n2.n_name AS nation,
--                     part.p_partkey AS tp_p_partkey,
--                     supplier.s_suppkey AS tp_s_suppkey,
--                     lineitem.l_orderkey AS tp_l_orderkey,
--                     lineitem.l_linenumber AS tp_l_linenumber,
--                     orders.o_orderkey AS tp_o_orderkey,
--                     customer.c_custkey AS tp_c_custkey,
--                     n1.n_nationkey AS tp_n_nationkey,
--                     n2.n_nationkey AS tp_n_nationkey,
--                     region.r_regionkey AS tp_r_regionkey
--                 FROM
--                     part,
--                     supplier,
--                     lineitem,
--                     orders,
--                     customer,
--                     nation n1,
--                     nation n2,
--                     region
--                 WHERE
--                     (
--                         (part.p_partkey = lineitem.l_partkey)
--                         AND (supplier.s_suppkey = lineitem.l_suppkey)
--                         AND (lineitem.l_orderkey = orders.o_orderkey)
--                         AND (orders.o_custkey = customer.c_custkey)
--                         AND (customer.c_nationkey = n1.n_nationkey)
--                         AND (n1.n_regionkey = region.r_regionkey)
--                         AND (region.r_name = 'AMERICA'::bpchar)
--                         AND (supplier.s_nationkey = n2.n_nationkey)
--                         AND (
--                             (orders.o_orderdate >= '1995-01-01'::date)
--                             AND (orders.o_orderdate <= '1996-12-31'::date)
--                         )
--                         AND (
--                             (part.p_type)::text = 'ECONOMY ANODIZED STEEL'::text
--                         )
--                     )
--             ) all_nations (
--                 o_year,
--                 volume,
--                 nation,
--                 tp_p_partkey,
--                 tp_s_suppkey,
--                 tp_l_orderkey,
--                 tp_l_linenumber,
--                 tp_o_orderkey,
--                 tp_c_custkey,
--                 tp_n_nationkey,
--                 tp_n_nationkey_1,
--                 tp_r_regionkey
--             )
--         GROUP BY
--             all_nations.o_year
--         ORDER BY
--             all_nations.o_year
--     ) tp_table_0;
select
    o_year,
    sum(
        case
            when nation = 'BRAZIL' then volume
            else 0
        end
    ) / sum(volume) as mkt_share
from
    (
        select
            extract(
                year
                from
                    o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) as volume,
            n2.n_name as nation
        from
            part,
            supplier,
            lineitem,
            orders,
            customer,
            nation n1,
            nation n2,
            region
        where
            p_partkey = l_partkey
            and s_suppkey = l_suppkey
            and l_orderkey = o_orderkey
            and o_custkey = c_custkey
            and c_nationkey = n1.n_nationkey
            and n1.n_regionkey = r_regionkey
            and (
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                o_orderkey,
                c_custkey,
                n1.n_nationkey,
                n2.n_nationkey,
                r_regionkey
            ) in (
                select
                    col1,
                    col2,
                    col3,
                    col4,
                    col5,
                    col6,
                    col7,
                    col8,
                    col9
                from
                    traceprov_perform_derivation (2, false) as (
                        col0 bigint,
                        col1 bigint,
                        col2 bigint,
                        col3 bigint,
                        col4 bigint,
                        col5 bigint,
                        col6 bigint,
                        col7 bigint,
                        col8 bigint,
                        col9 bigint
                    )
            )
    ) as all_nations
group by
    o_year
order by
    o_year;