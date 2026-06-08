-- traceprov_lineage_1
select t_17.out_index,
    t_12.in_index as column_1_1
from lineage_view(QID, 17) t_17
    join lineage_view(QID, 13) t_13 on t_17.in_index = t_13.out_index
    join lineage_view(QID, 12) t_12 on t_13.in_index = t_12.out_index;
-- traceprov_lineage_2
select t_9.out_index,
    t_0.in_index as column_1_1
from lineage_view(QID, 9) t_9
    join lineage_view(QID, 5) t_5 on t_9.lhs_index = t_5.out_index
    join lineage_view(QID, 2) t_2 on t_5.in_index = t_2.out_index
    join lineage_view(QID, 0) t_0 on t_2.lhs_index = t_0.out_index;
-- traceprov_lineage_5
select t_29.out_index,
    t_24.in_index as column_0,
    t_25.in_index as column_2
from lineage_view(QID, 29) t_29
    join lineage_view(QID, 27) t_27 on t_29.in_index = t_27.out_index
    join lineage_view(QID, 26) t_26 on t_27.rhs_index = t_26.out_index
    join lineage_view(QID, 24) t_24 on t_26.lhs_index = t_24.out_index
    join lineage_view(QID, 25) t_25 on t_26.rhs_index = t_25.out_index;
-- traceprov_lineage_4
select t_21.out_index,
    t_11.in_index as column_1_1
from lineage_view(QID, 21) t_21
    join lineage_view(QID, 9) t_9 on t_21.in_index = t_9.out_index
    join lineage_view(QID, 19) t_19 on t_9.rhs_index = t_19.out_index
    join lineage_view(QID, 11) t_11 on t_19.lhs_index = t_11.out_index;