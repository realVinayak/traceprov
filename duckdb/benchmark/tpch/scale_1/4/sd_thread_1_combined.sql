-- traceprov_lineage_2
select t_15.out_index,
    t_8.in_index as column_1
from lineage_view(QID, 15) t_15
    join lineage_view(QID, 12) t_12 on t_15.in_index = t_12.out_index
    join lineage_view(QID, 6) t_6 on t_12.in_index = t_6.out_index
    join lineage_view(QID, 8) t_8 on t_6.rhs_index = t_8.out_index;
-----------------------------
-- traceprov_lineage_1
select t_0.in_index as column_1_1,
    t_8.in_index as column_1_0
from lineage_view(QID, 3) t_3
    join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index
    join lineage_view(QID, 0) t_0 on t_1.in_index = t_0.out_index
    join lineage_view(QID, 8) t_8 on t_3.rhs_index = t_8.out_index;