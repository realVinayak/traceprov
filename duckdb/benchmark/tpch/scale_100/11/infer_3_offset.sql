SELECT
    tp_table_8.column_0,
    tp_table_9.column_1,
    tp_table_9.column_2,
    tp_table_9.column_3,
    tp_table_9.column_4
FROM
    (
        SELECT
            top_level_tp_table_0.column_0::bigint
        FROM
            traceprov_read_worker_layer_offset (1::bigint, 4::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
    ) as tp_table_8 (column_0)
    JOIN (
        SELECT
            intermediate_join_tp_table_1.column_0::bigint,
            intermediate_join_tp_table_1.column_1::bigint,
            intermediate_join_tp_table_1.column_2::bigint,
            intermediate_join_tp_table_1.column_3::bigint,
            intermediate_join_tp_table_1.column_4::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 3::bigint) AS intermediate_join_tp_table_1
    ) as tp_table_9 (column_0, column_1, column_2, column_3, column_4) ON (tp_table_8.column_0 = tp_table_9.column_0)