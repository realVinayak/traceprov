with result_map as (
    select *
    from (
            select category,
                "parallel",
                query_num_num_rows,
                query_num_group_num,
                avg(phase_1_profile_latency) as phase_1_profile_latency,
                any_value(offset_independent_cost) as offset_independent_cost
            from (
                    select category,
                        "parallel",
                        "offset",
                        query_num_num_rows,
                        query_num_group_num,
                        median(phase_1_profile_latency) as phase_1_profile_latency,
                        any_value(
                            coalesce(extra_setup_cost, 0.0) + coalesce(extra_sql_time, 0.0)
                        ) as offset_independent_cost
                    from dumped_versioned_filtered
                    group by category,
                        "parallel",
                        "offset",
                        query_num_num_rows,
                        query_num_group_num
                )
            group by category,
                "parallel",
                query_num_num_rows,
                query_num_group_num
        )
    where "parallel" in (1, 4, 12)
        and query_num_num_rows = 10_000_000
    order by "parallel",
        query_num_num_rows,
        query_num_group_num,
        category
)
select avg(
        sd.offset_independent_cost / sd.phase_1_profile_latency
    ) as sd_phase_1,
    avg(
        tp.offset_independent_cost / tp.phase_1_profile_latency
    ) as tp_phase_1,
    avg(sd.offset_independent_cost) as sd_cost,
    avg(tp.offset_independent_cost) as tp_cost
from result_map as sd
    join result_map as tp using (
        "parallel",
        query_num_num_rows,
        query_num_group_num
    )
where sd.category = 'smokedduck'
    and tp.category = 'traceprov';