select t_2.out_index,
    t_0.in_index
from lineage_view(1, 2) t_2
    join lineage_view(1, 0) t_0 on t_2.lhs_index = t_0.out_index;
---
select t_2.out_index,
    t_1.in_index
from lineage_view(1, 2) t_2
    join lineage_view(1, 1) t_1 on t_2.rhs_index = t_1.out_index;