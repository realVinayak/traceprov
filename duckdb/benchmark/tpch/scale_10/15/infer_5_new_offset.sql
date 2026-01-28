SELECT
    top_level_tp_table_0.column_0::bigint,
    top_level_tp_table_0.column_1::bigint
FROM
    traceprov_read_worker_layer_offset (1::bigint, 5::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0