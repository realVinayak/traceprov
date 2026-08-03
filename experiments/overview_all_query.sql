select
    category,
    query_num,
    median(total_time) as total_time_median,
    stddev(total_time) as total_time_std,
    mean(total_time) as total_time_mean,
    count(*) as c,
    any_value(fail_reason) as fail_reason,
    stddev(total_time) / mean(total_time) as total_time_std_ratio
from
    (
        select
            category,
            query_num,
            coalesce(phase_1_profile_latency, 0) + coalesce(phase_2_profile_latency, 0) + coalesce(extra_postprocess_time, 0) + coalesce(extra_sql_time, 0) as total_time,
            fail_reason
        from
            dumped_versioned_filtered
        where
            "parallel" = 1
            and (_EXTRA_PREDICATE_)
    )
group by
    category,
    query_num
order by
    total_time_std_ratio desc