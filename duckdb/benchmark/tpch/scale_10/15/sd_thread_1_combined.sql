-- traceprov_lineage_5
select t_21.out_index,
    t_0.in_index as column_0
from lineage_view(QID, 21) t_21
    join lineage_view(QID, 18) t_18 on t_21.in_index = t_18.out_index
    join lineage_view(QID, 0) t_0 on t_18.lhs_index = t_0.out_index;
-- traceprov_lineage_1
select t_21.out_index,
    t_1.in_index as column_1_1
from lineage_view(QID, 21) t_21
    join lineage_view(QID, 18) t_18 on t_21.in_index = t_18.out_index
    join lineage_view(QID, 17) t_17 on t_18.rhs_index = t_17.out_index
    join lineage_view(QID, 4) t_4 on t_17.lhs_index = t_4.out_index
    join lineage_view(QID, 1) t_1 on t_4.in_index = t_1.out_index;
-- traceprov_lineage_2
select t_9.out_index,
    t_6.in_index as column_1_1
from lineage_view(QID, 9) t_9
    join lineage_view(QID, 6) t_6 on t_9.in_index = t_6.out_index;