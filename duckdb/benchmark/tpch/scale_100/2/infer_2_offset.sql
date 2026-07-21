SELECT
    tp_table_9.column_0,
    tp_table_9.column_1,
    tp_table_9.column_2,
    tp_table_9.column_3,
    tp_table_9.column_4,
    tp_table_9.column_5,
    tp_table_10.column_0,
    tp_table_10.column_1
FROM
    (
        SELECT
            top_level_tp_table_0.column_0::bigint,
            top_level_tp_table_0.column_1::bigint,
            top_level_tp_table_0.column_2::bigint,
            top_level_tp_table_0.column_3::bigint,
            top_level_tp_table_0.column_4::bigint,
            top_level_tp_table_0.column_5::bigint
        FROM
            traceprov_read_worker_layer_offset (1::bigint, 3::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
    ) as tp_table_9 (
        column_0,
        column_1,
        column_2,
        column_3,
        column_4,
        column_5
    )
    JOIN (
        SELECT
            log_read_to_append_tp_table_1.column_0::bigint,
            log_read_to_append_tp_table_1.column_1::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 2::bigint) AS log_read_to_append_tp_table_1
    ) as tp_table_10 (column_0, column_1) ON (tp_table_9.column_0 = tp_table_10.column_0)