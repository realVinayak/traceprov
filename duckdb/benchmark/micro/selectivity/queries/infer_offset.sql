(
    SELECT
        tp_table_11.column_0,
        tp_table_12.column_1
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer_offset (1::bigint, 2::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
        ) as tp_table_11 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::int, 1::int) AS base_join_tp_table_1
        ) as tp_table_12 (column_0, column_1) ON (tp_table_11.column_0 = tp_table_12.column_0)
)