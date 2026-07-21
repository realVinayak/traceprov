-- does not work.
select t_24.out_index,
    t_5.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 22) t_22 on t_24.in_index = t_22.out_index
    join lineage_view(1, 19) t_19 on t_22.in_index = t_19.out_index
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 14) t_14 on t_16.rhs_index = t_14.out_index
    join lineage_view(1, 10) t_10 on t_14.in_index = t_10.out_index
    join lineage_view(1, 8) t_8 on t_10.in_index = t_8.out_index
    join lineage_view(1, 5) t_5 on t_8.in_index = t_5.out_index;
---
select t_24.out_index,
    t_0.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 22) t_22 on t_24.in_index = t_22.out_index
    join lineage_view(1, 19) t_19 on t_22.in_index = t_19.out_index
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 4) t_4 on t_16.lhs_index = t_4.out_index
    join lineage_view(1, 0) t_0 on t_4.lhs_index = t_0.out_index;
---
select t_24.out_index,
    t_1.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 22) t_22 on t_24.in_index = t_22.out_index
    join lineage_view(1, 19) t_19 on t_22.in_index = t_19.out_index
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 4) t_4 on t_16.lhs_index = t_4.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(1, 1) t_1 on t_3.lhs_index = t_1.out_index;
---
select t_24.out_index,
    t_2.in_index
from lineage_view(1, 24) t_24
    join lineage_view(1, 22) t_22 on t_24.in_index = t_22.out_index
    join lineage_view(1, 19) t_19 on t_22.in_index = t_19.out_index
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 4) t_4 on t_16.lhs_index = t_4.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(1, 2) t_2 on t_3.rhs_index = t_2.out_index;