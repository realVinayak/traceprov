with result_map as (
    select *
    from (
            select category,
                "parallel",
                query_num_num_rows,
                query_num_group_num,
                avg(phase_1_profile_latency) as phase_1_profile_latency
            from (
                    select category,
                        "parallel",
                        "offset",
                        query_num_num_rows,
                        query_num_group_num,
                        median(phase_1_profile_latency) as phase_1_profile_latency
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
        tp.phase_1_profile_latency / sd.phase_1_profile_latency
    )
from result_map as sd
    join result_map as tp using (
        "parallel",
        query_num_num_rows,
        query_num_group_num
    )
where sd.category = 'smokedduck'
    and tp.category = 'traceprov';