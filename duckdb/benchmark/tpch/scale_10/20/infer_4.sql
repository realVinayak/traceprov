SELECT
    top_level_tp_table_2.column_0::bigint,
    top_level_tp_table_2.column_1::bigint
FROM
    traceprov_read_worker_layer (1::bigint, 4::bigint) AS top_level_tp_table_2