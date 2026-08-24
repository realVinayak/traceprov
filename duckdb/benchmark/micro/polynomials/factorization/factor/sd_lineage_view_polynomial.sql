select
    (
        select
            '(' || string_agg(t_11.in_index :: text, ' + ') || ')' as table_prov_0
        from
            lineage_view(QID, 16) t_16
            join lineage_view(QID, 14) t_14 on t_16.rhs_index = t_14.out_index
            join lineage_view(QID, 11) t_11 on t_14.in_index = t_11.out_index
    ) || ' ⊗ ' || (
        select
            '(' || string_agg(t_0.in_index :: text, ' + ') || ')' as table_prov_1
        from
            lineage_view(QID, 16) t_16
            join lineage_view(QID, 10) t_10 on t_16.lhs_index = t_10.out_index
            join lineage_view(QID, 3) t_3 on t_10.lhs_index = t_3.out_index
            join lineage_view(QID, 0) t_0 on t_3.in_index = t_0.out_index
    ) || ' ⊗ ' || (
        select
            '(' || string_agg(t_5.in_index :: text, ' + ') || ')' as table_prov_2
        from
            lineage_view(QID, 16) t_16
            join lineage_view(QID, 10) t_10 on t_16.lhs_index = t_10.out_index
            join lineage_view(QID, 8) t_8 on t_10.rhs_index = t_8.out_index
            join lineage_view(QID, 5) t_5 on t_8.in_index = t_5.out_index
    ) as polynomial