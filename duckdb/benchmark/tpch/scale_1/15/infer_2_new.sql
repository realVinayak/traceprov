(
    SELECT
        tp_table_23.column_0,
        tp_table_23.column_1,
        tp_table_26.column_1
    FROM
        (
            SELECT
                tp_table_24.column_0,
                tp_table_25.column_1
            FROM
                (
                    SELECT
                        top_level_tp_table_7.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 4::bigint) AS top_level_tp_table_7
                ) as tp_table_24 (column_0)
                JOIN (
                    SELECT
                        intermediate_join_tp_table_8.column_0::bigint,
                        intermediate_join_tp_table_8.column_1::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 3::bigint) AS intermediate_join_tp_table_8
                ) as tp_table_25 (column_0, column_1) ON (tp_table_24.column_0 = tp_table_25.column_0)
        ) as tp_table_23 (column_0, column_1)
        JOIN (
            SELECT
                base_join_tp_table_9.column_0::bigint,
                base_join_tp_table_9.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS base_join_tp_table_9
        ) as tp_table_26 (column_0, column_1) ON (tp_table_23.column_1 = tp_table_26.column_0)
)