SELECT
    tp_table_5.column_0,
    tp_table_5.column_1,
    tp_table_5.column_2,
    tp_table_5.column_3,
    tp_table_10.column_1
FROM
    (
        SELECT
            tp_table_14.column_0,
            tp_table_14.column_1,
            tp_table_15.column_0,
            tp_table_15.column_1
        FROM
            (
                SELECT
                    intermediate_join_tp_table_1.column_0,
                    intermediate_join_tp_table_1.column_1
                FROM
                    traceprov_read_worker_layer (1::bigint, 3::bigint) AS intermediate_join_tp_table_1
            ) as tp_table_14 (column_0, column_1)
            JOIN (
                SELECT
                    log_read_to_append_tp_table_2.column_0::bigint,
                    log_read_to_append_tp_table_2.column_1::bigint
                FROM
                    traceprov_read_worker_layer (1::bigint, 2::bigint) AS log_read_to_append_tp_table_2
            ) as tp_table_15 (column_0, column_1) ON (tp_table_14.column_1 = tp_table_15.column_0)
    ) as tp_table_5 (column_0, column_1, column_2, column_3)
    JOIN (
        SELECT
            intermediate_join_tp_table_3.column_0::bigint,
            intermediate_join_tp_table_3.column_1::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_3
    ) as tp_table_10 (column_0, column_1) ON (tp_table_5.column_3 = tp_table_10.column_0)