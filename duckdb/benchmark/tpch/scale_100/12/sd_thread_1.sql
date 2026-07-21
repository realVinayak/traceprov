select t_9.out_index,
    t_0.in_index
from lineage_view(1, 9) t_9
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 3) t_3 on t_6.in_index = t_3.out_index
    join lineage_view(1, 0) t_0 on t_3.lhs_index = t_0.out_index;
---
select t_9.out_index,
    t_1.in_index
from lineage_view(1, 9) t_9
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 3) t_3 on t_6.in_index = t_3.out_index
    join lineage_view(1, 2) t_2 on t_3.rhs_index = t_2.out_index
    join lineage_view(1, 1) t_1 on t_2.in_index = t_1.out_index;
