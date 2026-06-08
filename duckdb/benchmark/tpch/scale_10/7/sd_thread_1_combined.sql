select t_19.out_index,
    t_0.in_index as column_2, --lineitem
    t_1.in_index as column_3, --orders,
    t_2.in_index as column_4, --customer
    t_3.in_index as column_6, --nation2
    t_7.in_index as column_1, --supplier,
    t_8.in_index as column_5 --nation1
from lineage_view(QID, 19) t_19
    join lineage_view(QID, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(QID, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(QID, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(QID, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(QID, 0) t_0 on t_6.lhs_index = t_0.out_index
    join lineage_view(QID, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(QID, 1) t_1 on t_5.lhs_index = t_1.out_index
    join lineage_view(QID, 4) t_4 on t_5.rhs_index = t_4.out_index
    join lineage_view(QID, 2) t_2 on t_4.lhs_index = t_2.out_index
    join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(QID, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(QID, 7) t_7 on t_9.lhs_index = t_7.out_index
    join lineage_view(QID, 8) t_8 on t_9.rhs_index = t_8.out_index;
