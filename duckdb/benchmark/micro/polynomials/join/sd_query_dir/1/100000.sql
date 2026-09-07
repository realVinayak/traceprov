select
    t_0.in_index as column_0,
    t_1.in_index as column_1
from
    lineage_view(QID, 2) t_2
    join lineage_view(QID, 0) t_0 on t_2.lhs_index = t_0.out_index
    join lineage_view(QID, 1) t_1 on t_2.rhs_index = t_1.out_index;

---
-- select
--     t_2.out_index,
--     t_1.in_index
-- from
--     lineage_view(QID, 2) t_2
--     join lineage_view(QID, 1) t_1 on t_2.rhs_index = t_1.out_index;
-- skew_1_0_num_100000: 0
-- polynomial_table_control_0: 1