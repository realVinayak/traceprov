with dumped_sample_normalized as (
    select
        category,
        query_num_group_num as query_num,
        parallel,
        "offset",
        any_value(fail_reason) as fail_reason,
        median(phase_1_profile_latency) as phase_1_latency,
        any_value(
            coalesce(extra_setup_cost, 0.0) + coalesce(extra_sql_time, 0.0)
        ) as offset_independent_cost,
        median(
            phase_2_profile_latency + coalesce(extra_partition_cost, 0.0)
        ) as phase_2_latency
    from
        dumped_versioned_filtered
    where
        category in ('traceprov', 'smokedduck')
        and query_num_num_rows = QUERY_NUM_ROWS
    group by
        category,
        query_num_group_num,
        parallel,
        "offset"
),
offset_normalized as (
    select
        category,
        dumped_sample_normalized.query_num,
        parallel,
        any_value(offset_independent_cost) as offset_independent_cost,
        cst.experiment as experiment_id,
        count(*) as repeat_count,
        sum(phase_2_latency) as phase_2_latency,
        any_value(phase_1_latency) as phase_1_latency,
        any_value(fail_reason) as fail_reason
    from
        dumped_sample_normalized
        join sample_offset as cst on cst.query_num = dumped_sample_normalized.query_num
        and cst.i_offset = dumped_sample_normalized.offset
    group by
        category,
        dumped_sample_normalized.query_num,
        cst.experiment,
        parallel
),
offset_exp_normalized as (
    select
        category,
        query_num,
        parallel,
        any_value(offset_independent_cost) as offset_independent_cost,
        any_value(phase_1_latency) as phase_1_latency,
        avg(phase_2_latency) as phase_2_latency,
        any_value(fail_reason) as fail_reason,
        avg(repeat_count) as repeat_count
    from
        offset_normalized
    group by
        category,
        query_num,
        parallel
)
select
    category,
    parallel,
    list(offset_exp_normalized)
from
    offset_exp_normalized
group by
    category,
    parallel;