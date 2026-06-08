select t_18.out_index,
    t_11.in_index as column_4,
    -- supplier
    t_0.in_index as column_3,
    -- lineitem
    t_1.in_index as column_2,
    -- orders
    t_2.in_index as column_1,
    -- customer
    t_3.in_index as column_5,
    -- nation
    t_4.in_index as column_6 -- region
from lineage_view(QID, 18) t_18
    join lineage_view(QID, 15) t_15 on t_18.in_index = t_15.out_index
    join lineage_view(QID, 12) t_12 on t_15.in_index = t_12.out_index
    join lineage_view(QID, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(QID, 10) t_10 on t_12.lhs_index = t_10.out_index
    join lineage_view(QID, 0) t_0 on t_10.lhs_index = t_0.out_index
    join lineage_view(QID, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(QID, 1) t_1 on t_9.lhs_index = t_1.out_index
    join lineage_view(QID, 7) t_7 on t_9.rhs_index = t_7.out_index
    join lineage_view(QID, 6) t_6 on t_7.in_index = t_6.out_index
    join lineage_view(QID, 2) t_2 on t_6.lhs_index = t_2.out_index
    join lineage_view(QID, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(QID, 3) t_3 on t_5.lhs_index = t_3.out_index
    join lineage_view(QID, 4) t_4 on t_5.rhs_index = t_4.out_index;