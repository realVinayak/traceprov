copy (select count(*) from ((
    SELECT
        tp_table_40.column_0,
        tp_table_41.column_1
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 4::bigint) AS top_level_tp_table_0
        ) as tp_table_40 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 3::bigint) AS base_join_tp_table_1
        ) as tp_table_41 (column_0, column_1) ON (tp_table_40.column_0 = tp_table_41.column_0)
)) f) to '/home/realvinayak123/projects/traceprov/duckdb/playground/tpch/queries/root/22/infer_3_count.csv'