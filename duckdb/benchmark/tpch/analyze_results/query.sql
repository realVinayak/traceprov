select category_label,
    list(
        struct_pack(
            query_num := query_num,
            capture_profile_latency := capture_profile_latency,
            base_profile_latency := base_profile_latency,
            capture_profile_stdev := capture_profile_stdev,
            base_profile_stdev := base_profile_stdev,
            average_time := average_time,
            max_stdev_ratio := max_stdev_ratio,
            max_stdev := max_stdev,
            relative_overhead := (
                (
                    (capture_profile_latency / base_profile_latency) - 1
                ) * 100
            )
        )
    )
from (
        select category as category_label,
            query_num,
            median(capture_profile_latency) as capture_profile_latency,
            median(base_profile_latency) as base_profile_latency,
            stddev(capture_profile_latency) / mean(capture_profile_latency) as capture_profile_stdev,
            stddev(base_profile_latency) / mean(base_profile_latency) as base_profile_stdev,
            any_value(average_time) as average_time,
            any_value(max_stdev_ratio) as max_stdev_ratio,
            any_value(max_stdev) as max_stdev
        from dumped
        where iter >= 6
        group by category,
            query_num
    )
group by category_label;