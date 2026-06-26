select category_label,
    list(
        struct_pack(
            query_num := query_num,
            capture_profile_latency := capture_profile_latency,
            base_profile_latency := base_profile_latency,
            capture_profile_stdev := capture_profile_stdev,
            base_profile_stdev := base_profile_stdev,
            average_time := coalesce(average_time, 0),
            average_time_with_index := coalesce(average_time, 0) + index_build_time_median,
            max_stdev_ratio := max_stdev_ratio,
            max_stdev := max_stdev,
            mean_stdev := mean_stdev,
            relative_overhead := (
                (
                    (capture_profile_latency / base_profile_latency) - 1
                ) * 100
            ),
            log_sizes_page_requested_size := log_sizes_page_requested_size,
            log_sizes_page_used_size := log_sizes_page_used_size,
            log_sizes_bytes_used_size := log_sizes_bytes_used_size,
            log_sizes_page_requested_size_stdev_mean_ratio := log_sizes_page_requested_size_stdev_mean_ratio,
            log_sizes_page_used_size_stdev_mean_ratio := log_sizes_page_used_size_stdev_mean_ratio,
            log_sizes_bytes_used_size_stdev_mean_ratio := log_sizes_bytes_used_size_stdev_mean_ratio,
            log_sizes_page_requested_size_stdev := log_sizes_page_requested_size_stdev,
            log_sizes_page_used_size_stdev := log_sizes_page_used_size_stdev,
            log_sizes_bytes_used_size_stdev := log_sizes_bytes_used_size_stdev,
            total_time := coalesce(capture_profile_latency, 0) + coalesce(average_time, 0) + index_build_time_median,
            total_time_std := sqrt(
                (capture_profile_stdev_raw ^ 2) + (max_stdev ^ 2) + (index_build_time_stdev ^ 2)
            )
        )
    )
from (
        select category as category_label,
            query_num,
            median(capture_profile_latency) as capture_profile_latency,
            median(base_profile_latency) as base_profile_latency,
            median(log_sizes_page_requested_size) as log_sizes_page_requested_size,
            median(log_sizes_page_used_size) as log_sizes_page_used_size,
            median(log_sizes_bytes_used_size) as log_sizes_bytes_used_size,
            stddev(capture_profile_latency) / mean(capture_profile_latency) as capture_profile_stdev,
            stddev(base_profile_latency) / mean(base_profile_latency) as base_profile_stdev,
            stddev(log_sizes_page_requested_size) / median(log_sizes_page_requested_size) as log_sizes_page_requested_size_stdev_mean_ratio,
            stddev(log_sizes_page_used_size) / median(log_sizes_page_used_size) as log_sizes_page_used_size_stdev_mean_ratio,
            stddev(log_sizes_bytes_used_size) / median(log_sizes_bytes_used_size) as log_sizes_bytes_used_size_stdev_mean_ratio,
            stddev(log_sizes_page_requested_size) as log_sizes_page_requested_size_stdev,
            stddev(log_sizes_page_used_size) as log_sizes_page_used_size_stdev,
            stddev(log_sizes_bytes_used_size) as log_sizes_bytes_used_size_stdev,
            any_value(average_time) as average_time,
            any_value(max_stdev_ratio) as max_stdev_ratio,
            any_value(max_stdev) as max_stdev,
            any_value(mean_stdev) as mean_stdev,
            stddev(capture_profile_latency) as capture_profile_stdev_raw,
            median(extra_index_build_time) as index_build_time_median,
            stddev(extra_index_build_time) as index_build_time_stdev,
            any_value(sum_backtrace_time) as total_backtrace_time,
        from dumped
        where iter >= 6
        group by category,
            query_num
    )
group by category_label;