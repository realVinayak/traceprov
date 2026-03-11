select tp_table_5.column_0,
    tp_table_15.column_1,
    tp_table_15.column_2,
    tp_table_15.column_3,
    tp_table_15.column_4,
    tp_table_15.column_5,
    tp_table_15.column_6,
    tp_table_15.column_7,
    tp_table_15.column_8
FROM (
        SELECT unnest(
                traceprov_read_int_vector(top_level_tp_table_0.column_0, WORKER_ID)
            ) as column_0
        FROM traceprov_read_worker_layer(2::bigint, 0, 2::int) AS top_level_tp_table_0
    ) as tp_table_5(column_0)
    JOIN (
        select base_join_tp_table_1.column_0::bigint,
            base_join_tp_table_1.column_1::bigint,
            base_join_tp_table_1.column_2::bigint,
            base_join_tp_table_1.column_3::bigint,
            base_join_tp_table_1.column_4::bigint,
            base_join_tp_table_1.column_5::bigint,
            base_join_tp_table_1.column_6::bigint,
            base_join_tp_table_1.column_7::bigint,
            base_join_tp_table_1.column_8::bigint
        FROM traceprov_read_worker_layer (2::bigint, WORKER_ID::int, 1::int) AS base_join_tp_table_1
    ) as tp_table_15 (
        column_0,
        column_1,
        column_2,
        column_3,
        column_4,
        column_5,
        column_6,
        column_7,
        column_8
    ) ON (tp_table_5.column_0 = tp_table_15.column_0);