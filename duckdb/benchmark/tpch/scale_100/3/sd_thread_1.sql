-- #QUERY 0
select t_14.out_index,
    t_0.in_index
from lineage_view(1, 14) t_14
    JOIN lineage_view(1, 12) t_12 ON t_14.in_index = t_12.out_index
    JOIN lineage_view(1, 8) t_8 on t_12.in_index = t_8.out_index
    JOIN lineage_view(1, 5) t_5 on t_8.in_index = t_5.out_index
    JOIN lineage_view(1, 0) t_0 on t_0.out_index = t_5.lhs_index;
-- #QUERY 1
select t_14.out_index,
    t_1.in_index
from lineage_view(1, 14) t_14
    JOIN lineage_view(1, 12) t_12 ON t_14.in_index = t_12.out_index
    JOIN lineage_view(1, 8) t_8 on t_12.in_index = t_8.out_index
    JOIN lineage_view(1, 5) t_5 on t_8.in_index = t_5.out_index
    JOIN lineage_view(1, 4) t_4 on t_4.out_index = t_5.rhs_index
    JOIN lineage_view(1, 1) t_1 on t_1.out_index = t_4.lhs_index;
-- #QUERY 2
select t_14.out_index,
    t_3.in_index
from lineage_view(1, 14) t_14
    JOIN lineage_view(1, 12) t_12 ON t_14.in_index = t_12.out_index
    JOIN lineage_view(1, 8) t_8 on t_12.in_index = t_8.out_index
    JOIN lineage_view(1, 5) t_5 on t_8.in_index = t_5.out_index
    JOIN lineage_view(1, 4) t_4 on t_4.out_index = t_5.rhs_index
    JOIN lineage_view(1, 3) t_3 on t_3.out_index = t_4.rhs_index
    JOIN lineage_view(1, 2) t_2 on t_2.out_index = t_3.in_index;