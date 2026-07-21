SELECT
    tp_table_0.o_year,
    tp_table_0.mkt_share,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            all_nations.o_year,
            (
                sum(
                    CASE
                        WHEN (all_nations.nation = 'BRAZIL'::bpchar) THEN all_nations.volume
                        ELSE (0)::numeric
                    END
                ) / sum(all_nations.volume)
            ) AS mkt_share,
            traceprov_agg_key_parallel_offset_9 (
                1,
                (all_nations.tp_p_partkey)::bigint,
                (all_nations.tp_s_suppkey)::bigint,
                (all_nations.tp_l_orderkey)::bigint,
                (all_nations.tp_l_linenumber)::bigint,
                (all_nations.tp_o_orderkey)::bigint,
                (all_nations.tp_c_custkey)::bigint,
                (all_nations.tp_n_nationkey)::bigint,
                (all_nations.tp_n_nationkey_1)::bigint,
                (all_nations.tp_r_regionkey)::bigint
            ) AS mapped_agg
        FROM
            (
                SELECT
                    EXTRACT(
                        year
                        FROM
                            orders.o_orderdate
                    ) AS o_year,
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    ) AS volume,
                    n2.n_name AS nation,
                    part.p_partkey AS tp_p_partkey,
                    supplier.s_suppkey AS tp_s_suppkey,
                    lineitem.l_orderkey AS tp_l_orderkey,
                    lineitem.l_linenumber AS tp_l_linenumber,
                    orders.o_orderkey AS tp_o_orderkey,
                    customer.c_custkey AS tp_c_custkey,
                    n1.n_nationkey AS tp_n_nationkey,
                    n2.n_nationkey AS tp_n_nationkey,
                    region.r_regionkey AS tp_r_regionkey
                FROM
                    part,
                    supplier,
                    lineitem,
                    orders,
                    customer,
                    nation n1,
                    nation n2,
                    region
                WHERE
                    (
                        (part.p_partkey = lineitem.l_partkey)
                        AND (supplier.s_suppkey = lineitem.l_suppkey)
                        AND (lineitem.l_orderkey = orders.o_orderkey)
                        AND (orders.o_custkey = customer.c_custkey)
                        AND (customer.c_nationkey = n1.n_nationkey)
                        AND (n1.n_regionkey = region.r_regionkey)
                        AND (region.r_name = 'AMERICA'::bpchar)
                        AND (supplier.s_nationkey = n2.n_nationkey)
                        AND (
                            (orders.o_orderdate >= '1995-01-01'::date)
                            AND (orders.o_orderdate <= '1996-12-31'::date)
                        )
                        AND (
                            (part.p_type)::text = 'ECONOMY ANODIZED STEEL'::text
                        )
                    )
            ) all_nations (
                o_year,
                volume,
                nation,
                tp_p_partkey,
                tp_s_suppkey,
                tp_l_orderkey,
                tp_l_linenumber,
                tp_o_orderkey,
                tp_c_custkey,
                tp_n_nationkey,
                tp_n_nationkey_1,
                tp_r_regionkey
            )
        GROUP BY
            all_nations.o_year
        ORDER BY
            all_nations.o_year
    ) tp_table_0