-- does not work.
select t_21.out_index,
    t_0.in_index
from lineage_view(1, 21) t_21
    join lineage_view(1, 18) t_18 on t_21.in_index = t_18.out_index
    join lineage_view(1, 0) t_0 on t_18.lhs_index = t_0.out_index;
---
select t_21.out_index,
    t_1.in_index
from lineage_view(1, 21) t_21
    join lineage_view(1, 18) t_18 on t_21.in_index = t_18.out_index
    join lineage_view(1, 17) t_17 on t_18.rhs_index = t_17.out_index
    join lineage_view(1, 4) t_4 on t_17.lhs_index = t_4.out_index
    join lineage_view(1, 1) t_1 on t_4.in_index = t_1.out_index;
---
select t_21.out_index,
    t_6.in_index
from lineage_view(1, 21) t_21
    join lineage_view(1, 18) t_18 on t_21.in_index = t_18.out_index
    join lineage_view(1, 17) t_17 on t_18.rhs_index = t_17.out_index
    join lineage_view(1, 14) t_14 on t_17.rhs_index = t_14.out_index
    join lineage_view(1, 9) t_9 on t_14.in_index = t_9.out_index
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index;