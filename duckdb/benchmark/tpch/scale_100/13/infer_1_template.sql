SELECT tp_table_4.column_0,
    tp_table_4.column_1,
    tp_table_7.column_1,
    tp_table_7.column_2
FROM (
        SELECT tp_table_5.column_0,
            tp_table_6.column_1
        FROM (
                SELECT unnest(
                        traceprov_read_int_vector(top_level_tp_table_0.column_0, 0)
                    ) as column_0
                FROM traceprov_read_worker_layer (2::bigint, 0::bigint, 3::bigint) AS top_level_tp_table_0
            ) as tp_table_5 (column_0)
            JOIN (
                SELECT intermediate_join_tp_table_1.column_0::bigint,
                    unnest(
                        traceprov_read_int_vector(intermediate_join_tp_table_1.column_1, WORKER_ID)
                    ) as column_1
                FROM traceprov_read_worker_layer (2::bigint, 0::bigint, 2::bigint) AS intermediate_join_tp_table_1
            ) as tp_table_6 (column_0, column_1) ON (tp_table_5.column_0 = tp_table_6.column_0)
    ) as tp_table_4 (column_0, column_1)
    JOIN (
        SELECT intermediate_join_tp_table_2.column_0::bigint,
            intermediate_join_tp_table_2.column_1::bigint,
            intermediate_join_tp_table_2.column_2::bigint
        FROM traceprov_read_worker_layer (2::bigint, WORKER_ID::bigint, 1::bigint) AS intermediate_join_tp_table_2
    ) as tp_table_7 (column_0, column_1, column_2) ON (tp_table_4.column_1 = tp_table_7.column_0);