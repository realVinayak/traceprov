SELECT
    tp_table_12.column_0,
    tp_table_12.column_1,
    tp_table_12.column_2,
    tp_table_15.column_0,
    tp_table_15.column_1
FROM
    (
        SELECT
            tp_table_13.column_0,
            tp_table_14.column_1,
            tp_table_14.column_2
        FROM
            (
                SELECT
                    top_level_tp_table_0.column_0::bigint
                FROM
                    traceprov_read_worker_layer_offset (1::bigint, 4::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
            ) as tp_table_13 (column_0)
            JOIN (
                SELECT
                    intermediate_join_tp_table_1.column_0::bigint,
                    intermediate_join_tp_table_1.column_1::bigint,
                    intermediate_join_tp_table_1.column_2::bigint
                FROM
                    traceprov_read_worker_layer (1::bigint, 3::bigint) AS intermediate_join_tp_table_1
            ) as tp_table_14 (column_0, column_1, column_2) ON (tp_table_13.column_0 = tp_table_14.column_0)
    ) as tp_table_12 (column_0, column_1, column_2)
    JOIN (
        SELECT
            log_read_to_append_tp_table_2.column_0::bigint,
            log_read_to_append_tp_table_2.column_1::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 2::bigint) AS log_read_to_append_tp_table_2
    ) as tp_table_15 (column_0, column_1) ON (tp_table_12.column_2 = tp_table_15.column_0)