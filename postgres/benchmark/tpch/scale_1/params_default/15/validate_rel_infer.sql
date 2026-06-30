SELECT supplier.s_suppkey,
    supplier.s_name,
    supplier.s_address,
    supplier.s_phone,
    revenue0.total_revenue
FROM supplier,
    (
        SELECT lineitem.l_suppkey AS supplier_no,
            sum(
                (
                    lineitem.l_extendedprice * (1 - lineitem.l_discount)
                )
            ) AS total_revenue
        FROM lineitem
        WHERE (l_orderkey, l_linenumber) in (
                select col_2, col_3
                FROM traceprov_relation_infer_1_mat
            )
        GROUP BY lineitem.l_suppkey
    ) revenue0
WHERE (
        (supplier.s_suppkey = revenue0.supplier_no)
        AND (
            revenue0.total_revenue = (
                SELECT tp_table_1.max
                FROM (
                        SELECT max(revenue0_1.total_revenue) AS max
                        FROM (
                                SELECT lineitem.l_suppkey AS supplier_no,
                                    sum(
                                        (
                                            lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                                        )
                                    ) AS total_revenue
                                FROM lineitem
                                WHERE (l_orderkey, l_linenumber) in (
                                        select col_2, col_3
                                        from traceprov_relation_infer_2_mat
                                    )
                                GROUP BY lineitem.l_suppkey
                            ) revenue0_1
                    ) tp_table_1
            )
        )
    )
    and (
        s_suppkey in (
            select col_0
            FROM traceprov_relation_infer_5_mat
        )
    )
ORDER BY supplier.s_suppkey;