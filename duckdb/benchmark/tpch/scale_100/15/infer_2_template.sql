(
    SELECT tp_table_23.column_0,
        tp_table_23.column_1,
        tp_table_26.column_1
    FROM (
            SELECT tp_table_24.column_0,
                tp_table_25.column_1
            FROM (
                    SELECT unnest(
                            traceprov_read_int_vector(
                                top_level_tp_table_7.column_0::ubigint,
                                0::ubigint
                            )
                        ) as column_0
                    FROM traceprov_read_worker_layer (2::bigint, 0::bigint, 4::bigint) AS top_level_tp_table_7
                ) as tp_table_24 (column_0)
                JOIN (
                    SELECT intermediate_join_tp_table_8.column_0::bigint,
                        unnest(
                            traceprov_read_int_vector(intermediate_join_tp_table_8.column_1, WORKER_ID)
                        ) as column_1
                    FROM traceprov_read_worker_layer (2::bigint, 0::bigint, 3::bigint) AS intermediate_join_tp_table_8
                ) as tp_table_25 (column_0, column_1) ON (tp_table_24.column_0 = tp_table_25.column_0)
        ) as tp_table_23 (column_0, column_1)
        JOIN (
            SELECT base_join_tp_table_9.column_0::bigint,
                base_join_tp_table_9.column_1::bigint
            FROM traceprov_read_worker_layer (2::bigint, WORKER_ID::bigint, 2::bigint) AS base_join_tp_table_9
        ) as tp_table_26 (column_0, column_1) ON (tp_table_23.column_1 = tp_table_26.column_0)
);