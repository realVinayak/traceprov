select tp_table_5.column_0,
    tp_table_15.column_1
FROM (
        SELECT unnest(
                traceprov_read_int_vector(top_level_tp_table_0.column_0, 0)
            ) as column_0
        FROM traceprov_read_worker_layer(2::bigint, 0, 4::int) AS top_level_tp_table_0
    ) as tp_table_5(column_0)
    JOIN (
        select base_join_tp_table_1.column_0::bigint,
            base_join_tp_table_1.column_1::bigint
        FROM traceprov_read_worker_layer (2::bigint, 0::int, 3::int) AS base_join_tp_table_1
    ) as tp_table_15 (column_0, column_1) ON (tp_table_5.column_0 = tp_table_15.column_0);