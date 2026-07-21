select t_14.out_index,
    t_0.in_index as column_3,
    -- lineitem
    t_1.in_index as column_2,
    --orders
    t_2.in_index as column_1 --customer
from lineage_view(QID, 14) t_14
    JOIN lineage_view(QID, 12) t_12 ON t_14.in_index = t_12.out_index
    JOIN lineage_view(QID, 8) t_8 on t_12.in_index = t_8.out_index
    JOIN lineage_view(QID, 5) t_5 on t_8.in_index = t_5.out_index
    JOIN lineage_view(QID, 0) t_0 on t_0.out_index = t_5.lhs_index
    JOIN lineage_view(QID, 4) t_4 on t_4.out_index = t_5.rhs_index
    JOIN lineage_view(QID, 1) t_1 on t_1.out_index = t_4.lhs_index
    JOIN lineage_view(QID, 3) t_3 on t_3.out_index = t_4.rhs_index
    JOIN lineage_view(QID, 2) t_2 on t_2.out_index = t_3.in_index;