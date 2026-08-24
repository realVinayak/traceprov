SELECT
    tp_table_0."?column?",
    tp_table_0.mapped_agg,
    tp_table_0.mapped_agg_1 AS mapped_agg,
    tp_table_0.mapped_agg_2 AS mapped_agg,
    traceprov_log_entry_3(
        4,
        tp_table_0.mapped_agg,
        tp_table_0.mapped_agg_1,
        tp_table_0.mapped_agg_2
    ) AS tp_table_1
FROM
    (
        SELECT
            1 AS "?column?",
            unnamed_subquery.mapped_agg,
            unnamed_subquery_1.mapped_agg,
            unnamed_subquery_2.mapped_agg
        FROM
            (
                (
                    (
                        SELECT
                            polynomial_table_0.b,
                            traceprov_agg_key_parallel_offset_1(
                                1,
                                (polynomial_table_0.rowid) :: int
                            ) AS mapped_agg
                        FROM
                            polynomial_table_0
                        WHERE
                            (polynomial_table_0.id <= NUM)
                        GROUP BY
                            polynomial_table_0.b
                    ) unnamed_subquery
                    JOIN (
                        SELECT
                            polynomial_table_1.b,
                            traceprov_agg_key_parallel_offset_1(
                                2,
                                (polynomial_table_1.rowid) :: int
                            ) AS mapped_agg
                        FROM
                            polynomial_table_1
                        WHERE
                            (polynomial_table_1.id <= NUM)
                        GROUP BY
                            polynomial_table_1.b
                    ) unnamed_subquery_1 on unnamed_subquery.b = unnamed_subquery_1.b
                )
                JOIN (
                    SELECT
                        polynomial_table_2.b,
                        traceprov_agg_key_parallel_offset_1(
                            3,
                            (polynomial_table_2.rowid) :: int
                        ) AS mapped_agg
                    FROM
                        polynomial_table_2
                    WHERE
                        (polynomial_table_2.id <= NUM)
                    GROUP BY
                        polynomial_table_2.b
                ) unnamed_subquery_2 on unnamed_subquery_1.b = unnamed_subquery_2.b
            )
    ) tp_table_0(
        "?column?",
        mapped_agg,
        mapped_agg_1,
        mapped_agg_2
    )