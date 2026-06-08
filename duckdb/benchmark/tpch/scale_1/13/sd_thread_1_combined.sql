select t_11.out_index,
    t_0.in_index as column_2,
    --orders
    t_2.in_index as column_1_1 --customer
from lineage_view(QID, 11) t_11
    join lineage_view(QID, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(QID, 6) t_6 on t_10.in_index = t_6.out_index
    join lineage_view(QID, 3) t_3 on t_6.in_index = t_3.out_index
    left join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index
    left join lineage_view(QID, 0) t_0 on t_1.in_index = t_0.out_index
    left join lineage_view(QID, 2) t_2 on t_3.rhs_index = t_2.out_index;