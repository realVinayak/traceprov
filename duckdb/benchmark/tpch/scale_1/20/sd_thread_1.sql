select t_29.out_index,
    t_0.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
    join lineage_view(1, 21) t_21 on t_27.lhs_index = t_21.out_index
    join lineage_view(1, 9) t_9 on t_21.in_index = t_9.out_index
    join lineage_view(1, 5) t_5 on t_9.lhs_index = t_5.out_index
    join lineage_view(1, 2) t_2 on t_5.in_index = t_2.out_index
    join lineage_view(1, 0) t_0 on t_2.lhs_index = t_0.out_index;
---
-- select t_29.out_index,
--     t_11.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
--     join lineage_view(1, 21) t_21 on t_27.lhs_index = t_21.out_index
--     join lineage_view(1, 9) t_9 on t_21.in_index = t_9.out_index
--     join lineage_view(1, 5) t_5 on t_9.lhs_index = t_5.out_index
--     join lineage_view(1, 2) t_2 on t_5.in_index = t_2.out_index
--     join lineage_view(1, 10) t_10 on t_2.rhs_index = t_10.out_index
--     join lineage_view(1, 19) t_19 on t_10.in_index = t_19.out_index
--     join lineage_view(1, 11) t_11 on t_19.lhs_index = t_11.out_index;
-- ---
-- select t_29.out_index,
--     t_12.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
--     join lineage_view(1, 21) t_21 on t_27.lhs_index = t_21.out_index
--     join lineage_view(1, 9) t_9 on t_21.in_index = t_9.out_index
--     join lineage_view(1, 5) t_5 on t_9.lhs_index = t_5.out_index
--     join lineage_view(1, 2) t_2 on t_5.in_index = t_2.out_index
--     join lineage_view(1, 10) t_10 on t_2.rhs_index = t_10.out_index
--     join lineage_view(1, 19) t_19 on t_10.in_index = t_19.out_index
--     join lineage_view(1, 17) t_17 on t_19.rhs_index = t_17.out_index
--     join lineage_view(1, 13) t_13 on t_17.in_index = t_13.out_index
--     join lineage_view(1, 12) t_12 on t_13.in_index = t_12.out_index;
-- ---
-- select t_29.out_index,
--     t_11.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
--     join lineage_view(1, 21) t_21 on t_27.lhs_index = t_21.out_index
--     join lineage_view(1, 9) t_9 on t_21.in_index = t_9.out_index
--     join lineage_view(1, 19) t_19 on t_9.rhs_index = t_19.out_index
--     join lineage_view(1, 11) t_11 on t_19.lhs_index = t_11.out_index;
-- ---
-- select t_29.out_index,
--     t_12.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
--     join lineage_view(1, 21) t_21 on t_27.lhs_index = t_21.out_index
--     join lineage_view(1, 9) t_9 on t_21.in_index = t_9.out_index
--     join lineage_view(1, 19) t_19 on t_9.rhs_index = t_19.out_index
--     join lineage_view(1, 17) t_17 on t_19.rhs_index = t_17.out_index
--     join lineage_view(1, 13) t_13 on t_17.in_index = t_13.out_index
--     join lineage_view(1, 12) t_12 on t_13.in_index = t_12.out_index;
---
select t_29.out_index,
    t_24.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
    join lineage_view(1, 26) t_26 on t_27.rhs_index = t_26.out_index
    join lineage_view(1, 24) t_24 on t_26.lhs_index = t_24.out_index;
---
select t_29.out_index,
    t_25.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 27) t_27 on t_29.in_index = t_27.out_index
    join lineage_view(1, 26) t_26 on t_27.rhs_index = t_26.out_index
    join lineage_view(1, 25) t_25 on t_26.rhs_index = t_25.out_index;