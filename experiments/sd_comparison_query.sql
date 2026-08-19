with data_offset_mapped as (
    select
        category,
        query_num,
        any_value(phase_1_profile_latency) as phase_1_profile_latency_time,
        mean(phase_2_profile_latency_median) as phase_2_profile_latency_time,
        any_value(fail_reason) as fail_reason
    from
        (
            select
                category,
                query_num,
                median(phase_1_profile_latency) as phase_1_profile_latency,
                median(phase_2_profile_latency) as phase_2_profile_latency_median,
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
                        any_value(fail_reason) as fail_reason
                    from
                        data_offset.dumped_versioned_filtered
                    where
                        "parallel" = 1
                        and category in ('traceprov', 'smokedduck')
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
    group by
        category,
        query_num
),
data_result_base as (
    select
        median(phase_1_profile_latency) as baseline_phase_1_profile_latency,
        query_num
    from
        data_all.dumped_versioned_filtered
    where
        category = 'base'
        and parallel = 1
    group by
        query_num
),
data_result_extra as (
    select
        data_offset_mapped.*,
        (
            (
                data_offset_mapped.phase_1_profile_latency_time / data_result_base.baseline_phase_1_profile_latency
            ) - 1
        ) * 100 as phase_1_relative_overhead,
        data_offset_mapped.phase_1_profile_latency_time - data_result_base.baseline_phase_1_profile_latency as phase_1_absolute_overhead
    from
        data_offset_mapped
        join data_result_base on data_offset_mapped.query_num = data_result_base.query_num
)
select
    category,
    list(data_result_extra)
from
    data_result_extra
group by
    data_result_extra.category;