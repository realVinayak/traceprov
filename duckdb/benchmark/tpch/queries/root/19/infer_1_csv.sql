copy ((
    SELECT
        tp_table_11.column_0,
        tp_table_12.column_1,
        tp_table_12.column_2,
        tp_table_12.column_3
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_11 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint,
                base_join_tp_table_1.column_3::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_12 (column_0, column_1, column_2, column_3) ON (tp_table_11.column_0 = tp_table_12.column_0)
)) to '/home/realvinayak123/projects/traceprov/duckdb/playground/tpch/queries/root/19/infer_1_out.csv'