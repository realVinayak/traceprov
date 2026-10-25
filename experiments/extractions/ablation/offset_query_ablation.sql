with  ablation_table as ( select
    category,
    query_num,
    mean(phase_2_profile_latency) as phase_2_profile_latency
FROM
    (
        select
            parallel,
            category,
            query_num,
            coalesce(phase_2_profile_latency_median, 0.0) as phase_2_profile_latency
        from
            (
                select
                    parallel,
                    category,
                    query_num,
                    median(phase_2_profile_latency) as phase_2_profile_latency_median,
                    any_value(phase_2_row_count) as phase_2_row_count
                from
                    (
                        select
                            parallel,
                            category,
                            query_num,
                            "offset",
                            "iter",
                            sum(phase_2_profile_latency) as phase_2_profile_latency,
                            sum(phase_2_row_count) as phase_2_row_count
                        from
                            dumped_versioned_filtered
                        where
                            (parallel = 1 and category like 'traceprov%')
                        group by
                            parallel,
                            category,
                            query_num,
                            "offset",
                            "iter"
                    )
                group by
                    parallel,
                    category,
                    query_num,
                    "offset"
            )
    )
group by
    category,
    query_num
) select category, list(ablation_table) from ablation_table group by category;