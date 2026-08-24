select
    t_3.in_index :: text || ' ⊗ ' || t_0.in_index :: text || ' ⊗ ' || t_1.in_index :: text as polynomial
from
    lineage_view(0, 8) t_8
    join lineage_view(0, 4) t_4 on t_8.in_index = t_4.out_index
    join lineage_view(0, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(0, 2) t_2 on t_4.lhs_index = t_2.out_index
    join lineage_view(0, 0) t_0 on t_2.lhs_index = t_0.out_index
    join lineage_view(0, 1) t_1 on t_2.rhs_index = t_1.out_index;