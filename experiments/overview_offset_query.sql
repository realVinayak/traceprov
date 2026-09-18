select
    parallel,
    category,
    query_num,
    mean(total_time_offset) as total_usage_time,
    max(phase_2_profile_latency_std) / mean(total_time_offset) as total_time_std_ratio,
    any_value(fail_reason) as fail_reason
FROM
    (
        select
            parallel,
            category,
            query_num,
            phase_1_profile_latency + coalesce(phase_2_profile_latency_median, 0.0) + coalesce(extra_setup_time, 0.0) as total_time_offset,
            fail_reason,
            phase_2_profile_latency_std
        from
            (
                select
                    parallel,
                    category,
                    query_num,
                    median(phase_1_profile_latency) as phase_1_profile_latency,
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
                            parallel,
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
                            (_EXTRA_PREDICATE_)
                        group by
                            parallel,
                            category,
                            query_num,
                            "offset",
                            "iter"
                    )
                group by
                    parallel,
                    category,
                    query_num,
                    "offset"
            )
    )
group by
    parallel,
    category,
    query_num