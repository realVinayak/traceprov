-- does not work.
select t_24.out_index,
    t_9.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 21) t_21 on t_24.in_index = t_21.out_index
    join lineage_view(1, 17) t_17 on t_21.in_index = t_17.out_index
    join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index
    join lineage_view(1, 14) t_14 on t_16.rhs_index = t_14.out_index
    join lineage_view(1, 10) t_10 on t_14.in_index = t_10.out_index
    join lineage_view(1, 9) t_9 on t_10.in_index = t_9.out_index;
---
select t_24.out_index,
    t_0.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 21) t_21 on t_24.in_index = t_21.out_index
    join lineage_view(1, 17) t_17 on t_21.in_index = t_17.out_index
    join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index
    join lineage_view(1, 8) t_8 on t_16.lhs_index = t_8.out_index
    join lineage_view(1, 0) t_0 on t_8.lhs_index = t_0.out_index;
---
select t_24.out_index,
    t_1.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 21) t_21 on t_24.in_index = t_21.out_index
    join lineage_view(1, 17) t_17 on t_21.in_index = t_17.out_index
    join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index
    join lineage_view(1, 8) t_8 on t_16.lhs_index = t_8.out_index
    join lineage_view(1, 7) t_7 on t_8.rhs_index = t_7.out_index
    join lineage_view(1, 6) t_6 on t_7.in_index = t_6.out_index
    join lineage_view(1, 1) t_1 on t_6.lhs_index = t_1.out_index;