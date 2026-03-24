SELECT tp_table_0.c,
    tp_table_0.z,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1(2, tp_table_0.mapped_agg) AS tp_table_1
FROM (
        SELECT count(*) AS c,
            skew_1_0_num_ROW_COUNT.z,
            traceprov_agg_key_parallel_offset_1(1, (skew_1_0_num_ROW_COUNT.rowid)::int) AS mapped_agg
        FROM skew_1_0_num_ROW_COUNT
        GROUP BY skew_1_0_num_ROW_COUNT.z
        ORDER BY (count(*)) DESC
        LIMIT 10
    ) tp_table_0