SELECT
    tp_table_0."?column?",
    tp_table_0.mapped_agg,
    traceprov_log_entry_1(2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            1 AS "?column?",
            traceprov_agg_key_parallel_offset_3(
                1,
                (polynomial_table_0.rowid) :: int,
                (polynomial_table_1.rowid) :: int,
                (polynomial_table_2.rowid) :: int
            ) AS mapped_agg
        FROM
            polynomial_table_0
            join polynomial_table_1 on polynomial_table_0.b = polynomial_table_1.b
            join polynomial_table_2 on polynomial_table_1.b = polynomial_table_2.b
        where
            polynomial_table_0.id <= NUM
            and polynomial_table_1.id <= NUM
            and polynomial_table_2.id <= NUM
        GROUP BY
            1 :: integer
    ) tp_table_0