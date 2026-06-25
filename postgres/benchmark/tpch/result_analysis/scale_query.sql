with all_data as (
    select *, 1 as scale_factor from scale_1.normalized_slowdown 
UNION ALL
    select *, 10 as scale_factor from scale_10.normalized_slowdown
)
select scale_factor, category, list(F) from (
    select
    all_data.scale_factor,
    all_data.category,
    all_data.query_num,
    all_data.phase_total_time_median,
    all_data.phase_total_time_median / scale_1_res.phase_total_time_median as normalized_time,
    scale_1_res.phase_total_time_median as normalization_value
    from all_data join scale_1.normalized_slowdown as scale_1_res
    on scale_1_res.query_num=all_data.query_num and scale_1_res.category='base'
    order by all_data.query_num, all_data.scale_factor, all_data.category
) F group by scale_factor, category order by scale_factor, category;