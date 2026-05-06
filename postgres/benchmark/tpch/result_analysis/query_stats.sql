create OR REPLACE table normalized_stats AS (
        SELECT category,
            query_num,
            -- phase_1_explain_time
            mean(phase_1_explain_time) AS phase_1_explain_time_mean,
            median(phase_1_explain_time) AS phase_1_explain_time_median,
            stddev(phase_1_explain_time) AS phase_1_explain_time_stdev,
            -- phase_2_time
            mean(phase_2_time) AS phase_2_time_mean,
            median(phase_2_time) AS phase_2_time_median,
            stddev(phase_2_time) AS phase_2_time_stdev,
            -- phase_2_row_count
            mean(phase_2_row_count) AS phase_2_row_count_mean,
            median(phase_2_row_count) AS phase_2_row_count_median,
            stddev(phase_2_row_count) AS phase_2_row_count_stdev,
            -- log_size_page_requested_size
            mean(log_size_page_requested_size) AS log_size_page_requested_size_mean,
            median(log_size_page_requested_size) AS log_size_page_requested_size_median,
            stddev(log_size_page_requested_size) AS log_size_page_requested_size_stdev,
            -- log_size_page_used_size
            mean(log_size_page_used_size) AS log_size_page_used_size_mean,
            median(log_size_page_used_size) AS log_size_page_used_size_median,
            stddev(log_size_page_used_size) AS log_size_page_used_size_stdev,
            -- log_size_bytes_used_size
            mean(log_size_bytes_used_size) AS log_size_bytes_used_size_mean,
            median(log_size_bytes_used_size) AS log_size_bytes_used_size_median,
            stddev(log_size_bytes_used_size) AS log_size_bytes_used_size_stdev,
            -- phase_1 + phase_2 explain Time
            mean(phase_total_time) AS phase_total_time_mean,
            median(phase_total_time) AS phase_total_time_median,
            stddev(phase_total_time) AS phase_total_time_stdev
        FROM (
                select *,
                    coalesce(phase_1_explain_time, 0) + coalesce(phase_2_time, 0) as phase_total_time
                from normalized
            )
        GROUP BY category,
            query_num
        ORDER BY category,
            query_num
    );