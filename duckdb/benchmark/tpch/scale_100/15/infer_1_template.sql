(
    SELECT tp_table_82.column_0,
        tp_table_82.column_1,
        tp_table_83.column_1
    FROM (
            SELECT top_level_tp_table_0.column_0::bigint,
                unnest(
                    traceprov_read_int_vector(top_level_tp_table_0.column_1, WORKER_ID)
                ) as column_1
            FROM traceprov_read_worker_layer (2::bigint, 0::bigint, 5::bigint) AS top_level_tp_table_0
        ) as tp_table_82 (column_0, column_1)
        JOIN (
            SELECT base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint
            FROM traceprov_read_worker_layer (2::bigint, WORKER_ID::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_83 (column_0, column_1) ON (tp_table_82.column_1 = tp_table_83.column_0)
);