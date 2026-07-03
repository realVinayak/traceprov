with dumped_sample_normalized as (
    select
        category,
        layer_number,
        query_num,
        "offset",
        any_value(index_build_time) as index_build_time,
        median(latency) as latency
    from
        dumped_sample
    where
        iter_id >= 2
    group BY
        category,
        layer_number,
        query_num,
        "offset"
),
dumped_sample_normalized_layer as (
    select
        category,
        query_num,
        "offset",
        any_value(index_build_time) as index_build_time,
        sum(latency) as latency
    from
        dumped_sample_normalized
    group by
        category,
        query_num,
        "offset"
),
dumped_simpified as (
    select
        sd.query_num,
        sd.offset,
        sd.index_build_time,
        sd.latency as sd_latency,
        tp.latency as tp_latency,
        (sd.index_build_time / (tp.latency - sd.latency)) as breakeven
    from
        dumped_sample_normalized_layer sd
        join dumped_sample_normalized_layer tp on sd.query_num = tp.query_num
        and sd."offset" = tp."offset"
        and sd.category = 'SmokedDuck'
        and tp.category = 'optimized-n__threads-1__filter_pushdown-y__join_filter_rewrite-y__partition_in_agg-y'
)
select
    query_num,
    list(breakeven)
from
    dumped_simpified
group by
    query_num;