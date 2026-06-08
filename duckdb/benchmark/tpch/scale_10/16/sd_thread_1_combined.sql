-- traceprov_lineage_1
select t_24.out_index,
    t_0.in_index as column_1,
    --partsupp
    t_1.in_index as column_2 --part
from lineage_view(QID, 24) t_24
    join lineage_view(QID, 21) t_21 on t_24.in_index = t_21.out_index
    join lineage_view(QID, 17) t_17 on t_21.in_index = t_17.out_index
    join lineage_view(QID, 8) t_8 on t_17.in_index = t_8.out_index
    join lineage_view(QID, 0) t_0 on t_8.lhs_index = t_0.out_index
    join lineage_view(QID, 7) t_7 on t_8.rhs_index = t_7.out_index
    join lineage_view(QID, 1) t_1 on t_7.in_index = t_1.out_index;