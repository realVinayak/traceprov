SELECT tp_table_2.c_name,
    tp_table_2.c_custkey,
    tp_table_2.o_orderkey,
    tp_table_2.o_orderdate,
    tp_table_2.o_totalprice,
    tp_table_2.sum,
    tp_table_2.mapped_agg,
    traceprov_log_entry_1 (4, tp_table_2.mapped_agg) AS tp_table_3
FROM (
        SELECT customer.c_name,
            customer.c_custkey,
            orders.o_orderkey,
            orders.o_orderdate,
            orders.o_totalprice,
            sum(lineitem.l_quantity) AS sum,
            traceprov_agg_key_parallel_offset_4 (
                3,
                (customer.c_custkey)::bigint,
                (orders.o_orderkey)::bigint,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint
            ) AS mapped_agg
        FROM customer,
            orders,
            lineitem
        WHERE (
                (
                    orders.o_orderkey IN (
                        SELECT tp_table_1.l_orderkey
                        FROM (
                                SELECT lineitem_1.l_orderkey,
                                    traceprov_log_entry_volatile_2 (
                                        2,
                                        (lineitem_1.l_orderkey)::bigint,
                                        traceprov_agg_key_parallel_offset_2 (
                                            1,
                                            (lineitem_1.l_orderkey)::bigint,
                                            (lineitem_1.l_linenumber)::bigint
                                        )
                                    ) AS tp_table_0
                                FROM lineitem lineitem_1
                                GROUP BY lineitem_1.l_orderkey
                                HAVING (sum(lineitem_1.l_quantity) > (300)::numeric)
                            ) tp_table_1
                    )
                )
                AND (customer.c_custkey = orders.o_custkey)
                AND (orders.o_orderkey = lineitem.l_orderkey)
            )
        GROUP BY customer.c_name,
            customer.c_custkey,
            orders.o_orderkey,
            orders.o_orderdate,
            orders.o_totalprice
        ORDER BY orders.o_totalprice DESC,
            orders.o_orderdate
        LIMIT 100
    ) tp_table_2