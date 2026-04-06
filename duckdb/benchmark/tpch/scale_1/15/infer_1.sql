(
    SELECT
        tp_table_82.column_0,
        tp_table_82.column_1,
        tp_table_83.column_1,
        tp_table_83.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint,
                top_level_tp_table_0.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 5::bigint) AS top_level_tp_table_0
        ) as tp_table_82 (column_0, column_1)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_83 (column_0, column_1, column_2) ON (tp_table_82.column_1 = tp_table_83.column_0)
)