select
    t_0.in_index as column_0,
    t_2.in_index as column_1,
    t_1.in_index as column_2
from
    lineage_view(QID, 4) t_4
    join lineage_view(QID, 0) t_0 on t_4.lhs_index = t_0.out_index
    join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index
    join lineage_view(QID, 2) t_2 on t_3.rhs_index = t_2.out_index;

---
-- select
--     t_4.out_index,
--     t_1.in_index
-- from
--     lineage_view(QID, 4) t_4
--     join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
--     join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index;
-- ---
-- select
--     t_4.out_index,
--     t_2.in_index
-- from
--     lineage_view(QID, 4) t_4
--     join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
--     join lineage_view(QID, 2) t_2 on t_3.rhs_index = t_2.out_index;
-- skew_1_0_num_50000: 0
-- polynomial_table_control_1: 1
-- polynomial_table_control_0: 2