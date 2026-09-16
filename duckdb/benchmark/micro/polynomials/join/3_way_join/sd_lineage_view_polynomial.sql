-- 0 -> skew
-- 1 -> control_1
-- 2 -> control_0
select
    t_0.in_index :: text || ' ⊗ ' || t_2.in_index :: text || ' ⊗ ' || t_1.in_index :: text as polynomial
from
    lineage_view(QID, 8) t_8
    join lineage_view(QID, 5) t_5 on t_8.in_index = t_5.out_index
    join lineage_view(QID, 0) t_0 on t_5.lhs_index = t_0.out_index
    join lineage_view(QID, 3) t_3 on t_5.rhs_index = t_3.out_index
    join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index
    join lineage_view(QID, 2) t_2 on t_3.rhs_index = t_2.out_index;