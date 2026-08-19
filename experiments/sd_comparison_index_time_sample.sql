with dumped_sample_normalized as (
    select
        category,
        layer_number,
        query_num,
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
        data_offset.current_sample_table
    where
        "parallel" = 1
        and category in ('traceprov', 'smokedduck')
    group by
        category,
        layer_number,
        query_num,
        "offset"
),
dumped_sample_normalized_layer as (
    select
        category,
        dumped_sample_normalized.query_num,
        "offset",
        any_value(fail_reason) as fail_reason,
        any_value(offset_independent_cost) as offset_independent_cost,
        sum(phase_2_latency) as phase_2_latency,
        any_value(phase_1_latency) as phase_1_latency
    from
        dumped_sample_normalized
        left join (
            select
                unnest(
                    [TRACEPROV_EQ_LAYER_COMPARISON],
                    recursive := true
                )
        ) as active_layer(query_num, layer_number) on dumped_sample_normalized.query_num = active_layer.query_num
    where
        active_layer.query_num is NULL
        or (dumped_sample_normalized.layer_number = 0)
        or (
            active_layer.layer_number = dumped_sample_normalized.layer_number
        )
    group by
        category,
        dumped_sample_normalized.query_num,
        "offset"
),
offset_normalized as (
    select
        category,
        dumped_sample_normalized_layer.query_num,
        any_value(offset_independent_cost) as offset_independent_cost,
        cst.experiment as experiment_id,
        count(*) as repeat_count,
        sum(phase_2_latency) as phase_2_latency,
        any_value(phase_1_latency) as phase_1_latency,
        any_value(fail_reason) as fail_reason
    from
        dumped_sample_normalized_layer
        join data_all.sample_offset as cst on cst.query_num = dumped_sample_normalized_layer.query_num
        and cst.i_offset = dumped_sample_normalized_layer.offset
    group by
        category,
        dumped_sample_normalized_layer.query_num,
        cst.experiment
),
offset_exp_normalized as (
    select
        category,
        query_num,
        any_value(offset_independent_cost) as offset_independent_cost,
        any_value(phase_1_latency) as phase_1_latency,
        avg(phase_2_latency) as phase_2_latency,
        any_value(fail_reason) as fail_reason,
        avg(repeat_count) as repeat_count
    from
        offset_normalized
    group by
        category,
        query_num
)
select
    category,
    list(offset_exp_normalized)
from
    offset_exp_normalized
group by
    category;