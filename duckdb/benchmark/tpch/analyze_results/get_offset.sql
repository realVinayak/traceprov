with dumped_sample_normalized as (
    select
        category,
        layer_number,
        query_num,
        "offset",
        any_value(index_build_time) + any_value(coalesce(sql_time, 0)) as index_build_time,
        median(latency + coalesce(partition_time, 0)) as latency
    from
        dumped_sample
    where
        iter_id >= 5
    group BY
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
        any_value(index_build_time) as index_build_time,
        sum(latency) as latency
    from
        dumped_sample_normalized
        left join (
            select
                unnest(
                    [
            {q: '2', l: 3},
            {q: '4', l: 2},
            {q: '17', l: 3},
            {q: '18', l: 3},
            {q: '20', l: 5},
            {q: '21', l: 2},
            {q: '22', l: 3},
        ],
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
        any_value(index_build_time) as index_build_time,
        sum(latency) as latency,
        sample_offset.experiment as experiment_id,
        count(*) as repeat_count
    from
        dumped_sample_normalized_layer
        join sample_offset on sample_offset.query_num = dumped_sample_normalized_layer.query_num
        and sample_offset.i_offset = dumped_sample_normalized_layer.offset
    group by
        category,
        dumped_sample_normalized_layer.query_num,
        sample_offset.experiment
),
offset_exp_normalized as (
    select
        category,
        query_num,
        any_value(index_build_time) as index_build_time,
        avg(latency) as latency,
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