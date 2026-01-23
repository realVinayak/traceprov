SELECT
    tp_table_0.supp_nation,
    tp_table_0.cust_nation,
    tp_table_0.l_year,
    tp_table_0.revenue,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            shipping.supp_nation,
            shipping.cust_nation,
            shipping.l_year,
            sum(shipping.volume) AS revenue,
            traceprov_agg_key_parallel_offset_7 (
                1,
                (shipping.tp_s_suppkey)::bigint,
                (shipping.tp_l_orderkey)::bigint,
                (shipping.tp_l_linenumber)::bigint,
                (shipping.tp_o_orderkey)::bigint,
                (shipping.tp_c_custkey)::bigint,
                (shipping.tp_n_nationkey)::bigint,
                (shipping.tp_n_nationkey_1)::bigint
            ) AS mapped_agg
        FROM
            (
                SELECT
                    n1.n_name AS supp_nation,
                    n2.n_name AS cust_nation,
                    EXTRACT(
                        year
                        FROM
                            lineitem.l_shipdate
                    ) AS l_year,
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    ) AS volume,
                    supplier.s_suppkey AS tp_s_suppkey,
                    lineitem.l_orderkey AS tp_l_orderkey,
                    lineitem.l_linenumber AS tp_l_linenumber,
                    orders.o_orderkey AS tp_o_orderkey,
                    customer.c_custkey AS tp_c_custkey,
                    n1.n_nationkey AS tp_n_nationkey,
                    n2.n_nationkey AS tp_n_nationkey
                FROM
                    supplier,
                    lineitem,
                    orders,
                    customer,
                    nation n1,
                    nation n2
                WHERE
                    (
                        (supplier.s_suppkey = lineitem.l_suppkey)
                        AND (orders.o_orderkey = lineitem.l_orderkey)
                        AND (customer.c_custkey = orders.o_custkey)
                        AND (supplier.s_nationkey = n1.n_nationkey)
                        AND (customer.c_nationkey = n2.n_nationkey)
                        AND (
                            (
                                (n1.n_name = 'FRANCE'::bpchar)
                                AND (n2.n_name = 'GERMANY'::bpchar)
                            )
                            OR (
                                (n1.n_name = 'GERMANY'::bpchar)
                                AND (n2.n_name = 'FRANCE'::bpchar)
                            )
                        )
                        AND (
                            (lineitem.l_shipdate >= '1995-01-01'::date)
                            AND (lineitem.l_shipdate <= '1996-12-31'::date)
                        )
                    )
            ) shipping (
                supp_nation,
                cust_nation,
                l_year,
                volume,
                tp_s_suppkey,
                tp_l_orderkey,
                tp_l_linenumber,
                tp_o_orderkey,
                tp_c_custkey,
                tp_n_nationkey,
                tp_n_nationkey_1
            )
        GROUP BY
            shipping.supp_nation,
            shipping.cust_nation,
            shipping.l_year
        ORDER BY
            shipping.supp_nation,
            shipping.cust_nation,
            shipping.l_year
    ) tp_table_0