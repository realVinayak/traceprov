-- 0 -> skew
-- 1 -> control_0
-- 2 -> control_2
-- 3 -> control_1
select
    t_0.in_index :: text || ' ⊗ ' || t_1.in_index :: text || ' ⊗ ' || t_3.in_index :: text || ' ⊗ ' || t_2.in_index :: text as polynomial
from
    lineage_view(QID, 11) t_11
    join lineage_view(QID, 8) t_8 on t_11.in_index = t_8.out_index
    join lineage_view(QID, 0) t_0 on t_8.lhs_index = t_0.out_index
    join lineage_view(QID, 6) t_6 on t_8.rhs_index = t_6.out_index
    join lineage_view(QID, 1) t_1 on t_6.lhs_index = t_1.out_index
    join lineage_view(QID, 4) t_4 on t_6.rhs_index = t_4.out_index
    join lineage_view(QID, 2) t_2 on t_4.lhs_index = t_2.out_index
    join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index;