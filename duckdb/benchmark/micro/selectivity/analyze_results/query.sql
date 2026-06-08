select random,
    category,
    list(
        struct_pack(
            selectivity := selectivity,
            backtrace_time := backtrace_time,
            build_time := build_time,
            total_backtrace_time := total_backtrace_time
        )
    ) as obj
from (
        select random,
            category,
            selectivity,
            ANY_VALUE(average_time) as backtrace_time,
            ANY_VALUE(extra_index_build_time) as build_time,
            ANY_VALUE(average_time) + ANY_VALUE(extra_index_build_time) as total_backtrace_time
        from dumped
        where iter >= 6
        group by category,
            num_rows,
            random,
            selectivity
    ) F
group by random,
    category;