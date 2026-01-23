(
    SELECT
        tp_table_133.column_0,
        tp_table_134.column_1
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 3::bigint) AS top_level_tp_table_0
        ) as tp_table_133 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_10.column_0::bigint,
                base_join_tp_table_10.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS base_join_tp_table_10
        ) as tp_table_134 (column_0, column_1) ON (tp_table_133.column_0 = tp_table_134.column_0)
)