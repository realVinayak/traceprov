(
    SELECT
        tp_table_27.column_0,
        tp_table_28.column_1,
        tp_table_28.column_2,
        tp_table_28.column_3,
        tp_table_28.column_4,
        tp_table_28.column_5,
        tp_table_28.column_6,
        tp_table_28.column_7
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer_offset (1::bigint, 2::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
        ) as tp_table_27 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_3.column_0::bigint,
                base_join_tp_table_3.column_1::bigint,
                base_join_tp_table_3.column_2::bigint,
                base_join_tp_table_3.column_3::bigint,
                base_join_tp_table_3.column_4::bigint,
                base_join_tp_table_3.column_5::bigint,
                base_join_tp_table_3.column_6::bigint,
                base_join_tp_table_3.column_7::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_3
        ) as tp_table_28 (
            column_0,
            column_1,
            column_2,
            column_3,
            column_4,
            column_5,
            column_6,
            column_7
        ) ON (tp_table_27.column_0 = tp_table_28.column_0)
)