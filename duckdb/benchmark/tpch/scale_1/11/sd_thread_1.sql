select t_23.out_index,
    t_0.in_index
from lineage_view(1, 23) t_23
    join lineage_view(1, 20) t_20 on t_23.in_index = t_20.out_index
    join lineage_view(1, 7) t_7 on t_20.lhs_index = t_7.out_index
    join lineage_view(1, 4) t_4 on t_7.in_index = t_4.out_index
    join lineage_view(1, 0) t_0 on t_4.lhs_index = t_0.out_index;
---
select t_23.out_index,
    t_1.in_index
from lineage_view(1, 23) t_23
    join lineage_view(1, 20) t_20 on t_23.in_index = t_20.out_index
    join lineage_view(1, 7) t_7 on t_20.lhs_index = t_7.out_index
    join lineage_view(1, 4) t_4 on t_7.in_index = t_4.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(1, 1) t_1 on t_3.lhs_index = t_1.out_index;
---
select t_23.out_index,
    t_2.in_index
from lineage_view(1, 23) t_23
    join lineage_view(1, 20) t_20 on t_23.in_index = t_20.out_index
    join lineage_view(1, 7) t_7 on t_20.lhs_index = t_7.out_index
    join lineage_view(1, 4) t_4 on t_7.in_index = t_4.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(1, 2) t_2 on t_3.rhs_index = t_2.out_index;
---
select t_13.out_index,
    t_9.in_index
from lineage_view(1, 13) t_13
    join lineage_view(1, 9) t_9 on t_13.lhs_index = t_9.out_index;
---
select t_13.out_index,
    t_10.in_index
from lineage_view(1, 13) t_13
    join lineage_view(1, 12) t_12 on t_13.rhs_index = t_12.out_index
    join lineage_view(1, 10) t_10 on t_12.lhs_index = t_10.out_index;
---
select t_13.out_index,
    t_11.in_index
from lineage_view(1, 13) t_13
    join lineage_view(1, 12) t_12 on t_13.rhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index;