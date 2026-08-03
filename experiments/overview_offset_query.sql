select
    category,
    query_num,
    mean(total_time_offset) as total_time_mean,
    max(phase_2_profile_latency_std) / mean(total_time_offset) as std_ratio,
    any_value(fail_reason) as fail_reason
FROM
    (
        select
            category,
            query_num,
            phase_1_profile_latency + coalesce(phase_2_profile_latency_median, 0.0) + coalesce(extra_setup_time, 0.0) as total_time_offset,
            fail_reason,
            phase_2_profile_latency_std
        from
            (
                select
                    category,
                    query_num,
                    any_value(phase_1_profile_latency) as phase_1_profile_latency,
                    median(phase_2_profile_latency) as phase_2_profile_latency_median,
                    any_value(
                        coalesce(extra_setup_time, 0.0) + coalesce(extra_partition_time, 0.0) + coalesce(extra_setup_time, 0.0)
                    ) as extra_setup_time,
                    any_value(phase_2_row_count) as phase_2_row_count,
                    stddev(phase_2_profile_latency) / mean(phase_2_profile_latency) as phase_2_profile_latency_std_ratio,
                    stddev(phase_2_profile_latency) as phase_2_profile_latency_std,
                    any_value(fail_reason) as fail_reason
                from
                    (
                        select
                            category,
                            query_num,
                            "offset",
                            "iter",
                            any_value(phase_1_profile_latency) as phase_1_profile_latency,
                            sum(phase_2_profile_latency) as phase_2_profile_latency,
                            sum(phase_2_row_count) as phase_2_row_count,
                            any_value(fail_reason) as fail_reason,
                            any_value(extra_sql_time) as extra_sql_time,
                            any_value(extra_partition_cost) as extra_partition_time,
                            any_value(extra_setup_cost) as extra_setup_time
                        from
                            dumped_versioned_filtered
                        where
                            "parallel" = 1
                            and (_EXTRA_PREDICATE_)
                        group by
                            category,
                            query_num,
                            "offset",
                            "iter"
                    )
                group by
                    category,
                    query_num,
                    "offset"
            )
    )
group by
    category,
    query_num