SELECT tp_table_0.c_count,
    tp_table_0.custdist,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (3, tp_table_0.mapped_agg) AS tp_table_1
FROM (
        SELECT c_orders.c_count,
            count(*) AS custdist,
            traceprov_agg_key_parallel_offset_1 (2, c_orders.mapped_agg) AS mapped_agg
        FROM (
                SELECT customer.c_custkey,
                    count(orders.o_orderkey) AS count,
                    traceprov_agg_key_parallel_offset_2 (
                        1,
                        (customer.c_custkey)::bigint,
                        orders.o_orderkey::bigint
                    ) AS mapped_agg
                FROM (
                        customer
                        LEFT JOIN orders ON (
                            (
                                (customer.c_custkey = orders.o_custkey)
                                AND (
                                    (orders.o_comment)::text !~~ '%special%requests%'::text
                                )
                            )
                        )
                    )
                GROUP BY customer.c_custkey
            ) c_orders (c_custkey, c_count, mapped_agg)
        GROUP BY c_orders.c_count
        ORDER BY (count(*)) DESC,
            c_orders.c_count DESC
    ) tp_table_0