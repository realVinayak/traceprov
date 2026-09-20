select
    parallel,
    category,
    query_num_num_rows,
    query_num_col_count,
    median(total_time) as total_usage_time,
    stddev(total_time) as total_time_std,
    mean(total_time) as total_time_mean,
    count(*) as c,
    any_value(fail_reason) as fail_reason,
    stddev(total_time) / mean(total_time) as total_time_std_ratio,
    any_value(log_sizes_page_used_size) as log_sizes_page_used_size,
    any_value(log_sizes_bytes_used_size) as log_sizes_bytes_used_size
from
    (
        select
            parallel,
            category,
            query_num_num_rows,
            query_num_col_count,
            log_sizes_page_used_size,
            log_sizes_bytes_used_size,
            (
                case
                    when extra_total_time_set then extra_total_time
                    -- Be mindful of the fact that the time can never be null
                    when phase_1_profile_latency is NULL then NULL
                    else (
                        coalesce(phase_1_profile_latency, 0.0)
                    )
                end
            ) as total_time,
            fail_reason
        from
            dumped_versioned_filtered
    )
group by
    parallel,
    category,
    query_num_num_rows,
    query_num_col_count
order by
    total_time_std_ratio desc