SELECT
    base_join_tp_table_1.column_0::bigint as column_0,
    base_join_tp_table_1.column_1::bigint as column_1,
    base_join_tp_table_1.column_2::bigint as column_2,
    base_join_tp_table_1.column_3::bigint as column_3,
    base_join_tp_table_1.column_4::bigint as column_4,
    base_join_tp_table_1.column_5::bigint as column_5
FROM
    traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1