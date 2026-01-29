SELECT
    tp_table_5.column_0,
    tp_table_6.column_1,
    tp_table_6.column_2,
    tp_table_6.column_3,
    tp_table_6.column_4
FROM
    (
        SELECT
            top_level_tp_table_2.column_0::bigint
        FROM
            traceprov_read_worker_layer_offset (1::bigint, 2::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_2
    ) as tp_table_5 (column_0)
    JOIN (
        SELECT
            intermediate_join_tp_table_3.column_0::bigint,
            intermediate_join_tp_table_3.column_1::bigint,
            intermediate_join_tp_table_3.column_2::bigint,
            intermediate_join_tp_table_3.column_3::bigint,
            intermediate_join_tp_table_3.column_4::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_3
    ) as tp_table_6 (column_0, column_1, column_2, column_3, column_4) ON (tp_table_5.column_0 = tp_table_6.column_0)