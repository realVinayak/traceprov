select t_18.out_index,
    t_0.in_index as column_5,
    -- orders
    t_1.in_index as column_3,
    -- lineitem
    t_2.in_index as column_1,
    -- part
    t_6.in_index as column_4,
    -- partsupp
    t_7.in_index as column_2,
    -- supplier
    t_8.in_index as column_6 --nation
from lineage_view(QID, 18) t_18
    join lineage_view(QID, 15) t_15 on t_18.in_index = t_15.out_index
    join lineage_view(QID, 11) t_11 on t_15.in_index = t_11.out_index
    join lineage_view(QID, 5) t_5 on t_11.lhs_index = t_5.out_index
    join lineage_view(QID, 0) t_0 on t_5.lhs_index = t_0.out_index
    join lineage_view(QID, 4) t_4 on t_5.rhs_index = t_4.out_index
    join lineage_view(QID, 1) t_1 on t_4.lhs_index = t_1.out_index
    join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(QID, 2) t_2 on t_3.in_index = t_2.out_index
    join lineage_view(QID, 10) t_10 on t_11.rhs_index = t_10.out_index
    join lineage_view(QID, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(QID, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(QID, 7) t_7 on t_9.lhs_index = t_7.out_index
    join lineage_view(QID, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(QID, 8) t_8 on t_9.rhs_index = t_8.out_index;