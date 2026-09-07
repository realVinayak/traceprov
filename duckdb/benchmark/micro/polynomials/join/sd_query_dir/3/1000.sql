select
    t_4.in_index as column_0,
    t_3.in_index as column_1,
    t_1.in_index as column_2,
    t_0.in_index as column_3
from
    lineage_view(QID, 6) t_6
    join lineage_view(QID, 2) t_2 on t_6.lhs_index = t_2.out_index
    join lineage_view(QID, 0) t_0 on t_2.lhs_index = t_0.out_index
    join lineage_view(QID, 1) t_1 on t_2.rhs_index = t_1.out_index
    join lineage_view(QID, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(QID, 3) t_3 on t_5.lhs_index = t_3.out_index
    join lineage_view(QID, 4) t_4 on t_5.rhs_index = t_4.out_index;

---
-- select
--     t_6.out_index,
--     t_1.in_index
-- from
--     lineage_view(QID, 6) t_6
--     join lineage_view(QID, 2) t_2 on t_6.lhs_index = t_2.out_index
--     join lineage_view(QID, 1) t_1 on t_2.rhs_index = t_1.out_index;
-- ---
-- select
--     t_6.out_index,
--     t_3.in_index
-- from
--     lineage_view(QID, 6) t_6
--     join lineage_view(QID, 5) t_5 on t_6.rhs_index = t_5.out_index
--     join lineage_view(QID, 3) t_3 on t_5.lhs_index = t_3.out_index;
-- ---
-- select
--     t_6.out_index,
--     t_4.in_index
-- from
--     lineage_view(QID, 6) t_6
--     join lineage_view(QID, 5) t_5 on t_6.rhs_index = t_5.out_index
--     join lineage_view(QID, 4) t_4 on t_5.rhs_index = t_4.out_index;
-- polynomial_table_control_2: 0
-- polynomial_table_control_1: 1
-- polynomial_table_control_0: 3
-- skew_1_0_num_1000: 4