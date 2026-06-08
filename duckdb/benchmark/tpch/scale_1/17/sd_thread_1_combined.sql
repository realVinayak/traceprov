-- traceprov_lineage_1
select t_15.out_index,
    t_0.in_index as column_1_1 -- lineitem
from lineage_view(QID, 15) t_15
    join lineage_view(QID, 9) t_9 on t_15.in_index = t_9.out_index
    join lineage_view(QID, 5) t_5 on t_9.lhs_index = t_5.out_index
    join lineage_view(QID, 2) t_2 on t_5.in_index = t_2.out_index
    join lineage_view(QID, 0) t_0 on t_2.lhs_index = t_0.out_index;
--------------------------------------------------
-- traceprov_lineage_3
select t_15.out_index,
    t_11.in_index as column_1,
    -- lineitem
    t_12.in_index as column_2 -- part
from lineage_view(QID, 15) t_15
    join lineage_view(QID, 9) t_9 on t_15.in_index = t_9.out_index
    join lineage_view(QID, 13) t_13 on t_9.rhs_index = t_13.out_index
    join lineage_view(QID, 11) t_11 on t_13.lhs_index = t_11.out_index
    join lineage_view(QID, 12) t_12 on t_13.rhs_index = t_12.out_index;