SELECT tp_table_0.avg,
    tp_table_0.z,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1(2, tp_table_0.mapped_agg) AS tp_table_1
FROM (
        SELECT avg(skew_1_0_num_ROW_COUNT.val) AS avg,
            skew_1_0_num_ROW_COUNT.z,
            traceprov_agg_key_parallel_offset_1(1, (skew_1_0_num_ROW_COUNT.rowid)::int) AS mapped_agg
        FROM skew_1_0_num_ROW_COUNT
        GROUP BY skew_1_0_num_ROW_COUNT.z
        HAVING (avg(skew_1_0_num_ROW_COUNT.val) > (50)::numeric)
    ) tp_table_0