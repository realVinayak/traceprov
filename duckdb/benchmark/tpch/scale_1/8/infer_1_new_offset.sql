(
    SELECT
        tp_table_6.column_0,
        tp_table_9.column_1,
        tp_table_9.column_2,
        tp_table_9.column_3,
        tp_table_9.column_4,
        tp_table_9.column_5,
        tp_table_9.column_6,
        tp_table_9.column_7,
        tp_table_9.column_8
    FROM
        (
            SELECT
                tp_table_7.column_0,
                tp_table_8.column_1,
                tp_table_8.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer_offset (1::bigint, 2::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
                ) as tp_table_7 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_8 (column_0, column_1, column_2) ON (tp_table_7.column_0 = tp_table_8.column_0)
        ) as tp_table_6 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint,
                base_join_tp_table_1.column_3::bigint,
                base_join_tp_table_1.column_4::bigint,
                base_join_tp_table_1.column_5::bigint,
                base_join_tp_table_1.column_6::bigint,
                base_join_tp_table_1.column_7::bigint,
                base_join_tp_table_1.column_8::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_9 (
            column_0,
            column_1,
            column_2,
            column_3,
            column_4,
            column_5,
            column_6,
            column_7,
            column_8
        ) ON (
            tp_table_6.column_2 = tp_table_9.column_0
            AND tp_table_6.column_1 = 1
        )
)
UNION ALL
(
    SELECT
        tp_table_11.column_0,
        tp_table_12.column_1,
        tp_table_12.column_2,
        tp_table_12.column_3,
        tp_table_12.column_4,
        tp_table_12.column_5,
        tp_table_12.column_6,
        tp_table_12.column_7,
        tp_table_12.column_8
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
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint,
                base_join_tp_table_1.column_3::bigint,
                base_join_tp_table_1.column_4::bigint,
                base_join_tp_table_1.column_5::bigint,
                base_join_tp_table_1.column_6::bigint,
                base_join_tp_table_1.column_7::bigint,
                base_join_tp_table_1.column_8::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_12 (
            column_0,
            column_1,
            column_2,
            column_3,
            column_4,
            column_5,
            column_6,
            column_7,
            column_8
        ) ON (tp_table_11.column_0 = tp_table_12.column_0)
)