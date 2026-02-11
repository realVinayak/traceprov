SELECT
    intermediate_join_tp_table_3.column_0::bigint as column_0,
    intermediate_join_tp_table_3.column_1::bigint as column_1,
    intermediate_join_tp_table_3.column_2::bigint as column_2
FROM
    traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_3
where
    exists (
        SELECT
            top_level_tp_table_2.column_0::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_2
    )