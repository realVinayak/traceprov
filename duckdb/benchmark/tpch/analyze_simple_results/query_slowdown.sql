create or replace table normalized_slowdown as (
        select a.*,
            a.phase_1_explain_time_median / b.phase_1_explain_time_median as phase_1_slowdown,
            a.phase_total_time_median / b.phase_1_explain_time_median as phase_all_slowdown
        from normalized_stats a
            join normalized_stats b on a.query_num = b.query_num
            and b.category = 'base'
    );