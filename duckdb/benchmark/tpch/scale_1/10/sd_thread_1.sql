select t_15.out_index,
    t_0.in_index
from lineage_view(1, 15) t_15
    join lineage_view(1, 13) t_13 on t_15.in_index = t_13.out_index
    join lineage_view(1, 9) t_9 on t_13.in_index = t_9.out_index
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 0) t_0 on t_6.lhs_index = t_0.out_index;
---
select t_15.out_index,
    t_1.in_index
from lineage_view(1, 15) t_15
    join lineage_view(1, 13) t_13 on t_15.in_index = t_13.out_index
    join lineage_view(1, 9) t_9 on t_13.in_index = t_9.out_index
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 1) t_1 on t_5.lhs_index = t_1.out_index;
---
select t_15.out_index,
    t_2.in_index
from lineage_view(1, 15) t_15
    join lineage_view(1, 13) t_13 on t_15.in_index = t_13.out_index
    join lineage_view(1, 9) t_9 on t_13.in_index = t_9.out_index
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 4) t_4 on t_5.rhs_index = t_4.out_index
    join lineage_view(1, 2) t_2 on t_4.lhs_index = t_2.out_index;
---
select t_15.out_index,
    t_3.in_index
from lineage_view(1, 15) t_15
    join lineage_view(1, 13) t_13 on t_15.in_index = t_13.out_index
    join lineage_view(1, 9) t_9 on t_13.in_index = t_9.out_index
    join lineage_view(1, 6) t_6 on t_9.in_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 4) t_4 on t_5.rhs_index = t_4.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index;
