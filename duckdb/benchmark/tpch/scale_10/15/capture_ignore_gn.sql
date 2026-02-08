SELECT
    tp_table_2.s_suppkey,
    tp_table_2.s_name,
    tp_table_2.s_address,
    tp_table_2.s_phone,
    tp_table_2.total_revenue,
    traceprov_log_entry_2 (
        5,
        (tp_table_2.tp_s_suppkey),
        tp_table_2.mapped_agg
    ) AS tp_table_3
FROM
    (
        SELECT
            supplier.s_suppkey,
            supplier.s_name,
            supplier.s_address,
            supplier.s_phone,
            revenue0.total_revenue,
            supplier.rowid AS tp_s_suppkey,
            revenue0.mapped_agg
        FROM
            supplier,
            (
                SELECT
                    lineitem.l_suppkey AS supplier_no,
                    sum(
                        (
                            lineitem.l_extendedprice * ((1) - lineitem.l_discount)
                        )
                    ) AS total_revenue,
                    traceprov_agg_key_parallel_offset_1 (1, lineitem.rowid) AS mapped_agg
                FROM
                    lineitem
                WHERE
                    (
                        (lineitem.l_shipdate >= '1996-01-01'::date)
                        AND (
                            lineitem.l_shipdate < ('1996-01-01'::date + '3 mons'::interval month)
                        )
                    )
                GROUP BY
                    lineitem.l_suppkey
            ) revenue0
        WHERE
            (
                (supplier.s_suppkey = revenue0.supplier_no)
                AND (
                    revenue0.total_revenue = (
                        SELECT
                            tp_table_1.max
                        FROM
                            (
                                SELECT
                                    max(revenue0_1.total_revenue) AS max,
                                    traceprov_log_entry_volatile_1 (
                                        4,
                                        traceprov_agg_key_parallel_offset_ignore_gn_1 (3, revenue0_1.mapped_agg)
                                    ) AS tp_table_0
                                FROM
                                    (
                                        SELECT
                                            lineitem.l_suppkey AS supplier_no,
                                            sum(
                                                (
                                                    lineitem.l_extendedprice * ((1) - lineitem.l_discount)
                                                )
                                            ) AS total_revenue,
                                            traceprov_agg_key_parallel_offset_1 (2, lineitem.rowid) AS mapped_agg
                                        FROM
                                            lineitem
                                        WHERE
                                            (
                                                (lineitem.l_shipdate >= '1996-01-01'::date)
                                                AND (
                                                    lineitem.l_shipdate < ('1996-01-01'::date + '3 mons'::interval month)
                                                )
                                            )
                                        GROUP BY
                                            lineitem.l_suppkey
                                    ) revenue0_1
                            ) tp_table_1
                    )
                )
            )
        ORDER BY
            supplier.s_suppkey
    ) tp_table_2