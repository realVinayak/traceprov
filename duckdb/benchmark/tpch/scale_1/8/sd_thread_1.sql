select t_22.out_index,
    t_13.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 13) t_13 on t_14.rhs_index = t_13.out_index;
---
select t_22.out_index,
    t_0.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 0) t_0 on t_12.lhs_index = t_0.out_index;
---
select t_22.out_index,
    t_1.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 7) t_7 on t_11.lhs_index = t_7.out_index
    join lineage_view(1, 1) t_1 on t_7.lhs_index = t_1.out_index;
---
select t_22.out_index,
    t_2.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 7) t_7 on t_11.lhs_index = t_7.out_index
    join lineage_view(1, 6) t_6 on t_7.rhs_index = t_6.out_index
    join lineage_view(1, 2) t_2 on t_6.lhs_index = t_2.out_index;
---
select t_22.out_index,
    t_3.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 7) t_7 on t_11.lhs_index = t_7.out_index
    join lineage_view(1, 6) t_6 on t_7.rhs_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 3) t_3 on t_5.lhs_index = t_3.out_index;
---
select t_22.out_index,
    t_4.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 7) t_7 on t_11.lhs_index = t_7.out_index
    join lineage_view(1, 6) t_6 on t_7.rhs_index = t_6.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index
    join lineage_view(1, 4) t_4 on t_5.rhs_index = t_4.out_index;
---
select t_22.out_index,
    t_8.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.rhs_index = t_10.out_index
    join lineage_view(1, 8) t_8 on t_10.lhs_index = t_8.out_index;
---
select t_22.out_index,
    t_9.in_index
from lineage_view(1, 22) t_22
    join lineage_view(1, 18) t_18 on t_22.in_index = t_18.out_index
    join lineage_view(1, 14) t_14 on t_18.in_index = t_14.out_index
    join lineage_view(1, 12) t_12 on t_14.lhs_index = t_12.out_index
    join lineage_view(1, 11) t_11 on t_12.rhs_index = t_11.out_index
    join lineage_view(1, 10) t_10 on t_11.rhs_index = t_10.out_index
    join lineage_view(1, 9) t_9 on t_10.rhs_index = t_9.out_index;
