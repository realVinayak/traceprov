SELECT
    base_join_tp_table_1.column_0::bigint,
    base_join_tp_table_1.column_1::bigint
FROM
    traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
where
    exists (
        SELECT
            top_level_tp_table_0.column_0::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
    )