select
    parallel,
    category,
    query_num,
    median(total_time) as total_time_median,
    stddev(total_time) as total_time_std,
    mean(total_time) as total_time_mean,
    count(*) as c,
    any_value(fail_reason) as fail_reason,
    stddev(total_time) / mean(total_time) as total_time_std_ratio,
    median(backtrace_time) as backtrace_time_median,
    stddev(backtrace_time) as backtrace_time_std,
    mean(backtrace_time) as backtrace_time_mean,
    (stddev(backtrace_time) / mean(backtrace_time)) * 100 as backtrace_time_std_ratio,
from
    (
        select
            parallel,
            category,
            query_num,
            (
                case
                    when extra_total_time_set then extra_total_time
                    else (
                        coalesce(phase_1_profile_latency, 0.0) + coalesce(phase_2_profile_latency, 0.0) + coalesce(extra_postprocess_time, 0.0) + coalesce(extra_sql_time, 0.0)
                    )
                end
            ) as total_time,
            coalesce(phase_2_profile_latency, 0.0) + coalesce(extra_postprocess_time, 0.0) + coalesce(extra_sql_time, 0.0) as backtrace_time,
            fail_reason
        from
            dumped_versioned_filtered
        where
            (_EXTRA_PREDICATE_)
    )
group by
    parallel,
    category,
    query_num
order by
    total_time_std_ratio desc