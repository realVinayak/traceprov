----
-- traceprov_lineage_1
select t_14.in_index as column_1
from lineage_view(QID, 20) t_20
    join lineage_view(QID, 14) t_14 on t_20.in_index = t_14.out_index;
-- traceprov_lineage_3
select t_32.out_index,
    t_7.in_index as column_1
from lineage_view(QID, 32) t_32
    join lineage_view(QID, 31) t_31 on t_32.in_index = t_31.out_index
    join lineage_view(QID, 5) t_5 on t_31.in_index = t_5.out_index
    join lineage_view(QID, 27) t_27 on t_5.rhs_index = t_27.out_index
    join lineage_view(QID, 13) t_13 on t_13.out_index = t_27.lhs_index
    join lineage_view(QID, 7) t_7 on t_7.out_index = t_13.in_index;