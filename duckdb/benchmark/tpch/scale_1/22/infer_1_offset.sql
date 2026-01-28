(
    SELECT
        tp_table_15.column_0,
        tp_table_16.column_1
    FROM
        (
            SELECT
                top_level_tp_table_4.column_0::bigint
            FROM
                traceprov_read_worker_layer_offset (1::bigint, 2::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_4
        ) as tp_table_15 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_5.column_0::bigint,
                base_join_tp_table_5.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_5
        ) as tp_table_16 (column_0, column_1) ON (tp_table_15.column_0 = tp_table_16.column_0)
)