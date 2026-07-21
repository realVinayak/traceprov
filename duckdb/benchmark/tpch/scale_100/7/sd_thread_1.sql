select t_19.out_index,
    t_0.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(1, 2) t_2 on t_6.lhs_index = t_2.out_index
    join lineage_view(1, 0) t_0 on t_2.lhs_index = t_0.out_index;
---
select t_19.out_index,
    t_1.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(1, 2) t_2 on t_6.lhs_index = t_2.out_index
    join lineage_view(1, 1) t_1 on t_2.rhs_index = t_1.out_index;
---
select t_19.out_index,
    t_3.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 3) t_3 on t_5.lhs_index = t_3.out_index;
---
select t_19.out_index,
    t_4.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 6) t_6 on t_10.lhs_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 4) t_4 on t_5.rhs_index = t_4.out_index;
---
select t_19.out_index,
    t_7.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(1, 7) t_7 on t_9.lhs_index = t_7.out_index;
---
select t_19.out_index,
    t_8.in_index
from lineage_view(1, 19) t_19
    join lineage_view(1, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(1, 11) t_11 on t_16.in_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.in_index = t_10.out_index
    join lineage_view(1, 9) t_9 on t_10.rhs_index = t_9.out_index
    join lineage_view(1, 8) t_8 on t_9.rhs_index = t_8.out_index;
