-- select t_32.out_index,
--     t_7.in_index
-- from lineage_view(1, 32) t_32
--     join lineage_view(1, 31) t_31 on t_32.in_index = t_31.out_index
--     join lineage_view(1, 5) t_5 on t_31.in_index = t_5.out_index
--     join lineage_view(1, 27) t_27 on t_5.rhs_index = t_27.out_index
--     join lineage_view(1, 13) t_13 on t_27.lhs_index = t_13.out_index
--     join lineage_view(1, 12) t_12 on t_13.in_index = t_12.out_index
--     join lineage_view(1, 7) t_7 on t_12.lhs_index = t_7.out_index;
---
-- select t_32.out_index,
--     t_14.in_index
-- from lineage_view(1, 32) t_32
--     join lineage_view(1, 31) t_31 on t_32.in_index = t_31.out_index
--     join lineage_view(1, 5) t_5 on t_31.in_index = t_5.out_index
--     join lineage_view(1, 27) t_27 on t_5.rhs_index = t_27.out_index
--     join lineage_view(1, 24) t_24 on t_27.rhs_index = t_24.out_index
--     join lineage_view(1, 20) t_20 on t_24.in_index = t_20.out_index
--     join lineage_view(1, 19) t_19 on t_20.in_index = t_19.out_index
--     join lineage_view(1, 14) t_14 on t_19.lhs_index = t_14.out_index;
---
select t_32.out_index,
    t_0.in_index
from lineage_view(1, 32) t_32
    join lineage_view(1, 31) t_31 on t_32.in_index = t_31.out_index
    join lineage_view(1, 5) t_5 on t_31.in_index = t_5.out_index
    join lineage_view(1, 2) t_2 on t_5.lhs_index = t_2.out_index
    join lineage_view(1, 0) t_0 on t_2.lhs_index = t_0.out_index;
---
-- select t_32.out_index,
--     t_7.in_index
-- from lineage_view(1, 32) t_32
--     join lineage_view(1, 31) t_31 on t_32.in_index = t_31.out_index
--     join lineage_view(1, 5) t_5 on t_31.in_index = t_5.out_index
--     join lineage_view(1, 2) t_2 on t_5.lhs_index = t_2.out_index
--     join lineage_view(1, 6) t_6 on t_2.rhs_index = t_6.out_index
--     join lineage_view(1, 27) t_27 on t_6.in_index = t_27.out_index
--     join lineage_view(1, 13) t_13 on t_27.lhs_index = t_13.out_index
--     join lineage_view(1, 12) t_12 on t_13.in_index = t_12.out_index
--     join lineage_view(1, 7) t_7 on t_12.lhs_index = t_7.out_index;
---
-- select t_32.out_index,
--     t_14.in_index
-- from lineage_view(1, 32) t_32
--     join lineage_view(1, 31) t_31 on t_32.in_index = t_31.out_index
--     join lineage_view(1, 5) t_5 on t_31.in_index = t_5.out_index
--     join lineage_view(1, 2) t_2 on t_5.lhs_index = t_2.out_index
--     join lineage_view(1, 6) t_6 on t_2.rhs_index = t_6.out_index
--     join lineage_view(1, 27) t_27 on t_6.in_index = t_27.out_index
--     join lineage_view(1, 24) t_24 on t_27.rhs_index = t_24.out_index
--     join lineage_view(1, 20) t_20 on t_24.in_index = t_20.out_index
--     join lineage_view(1, 19) t_19 on t_20.in_index = t_19.out_index
--     join lineage_view(1, 14) t_14 on t_19.lhs_index = t_14.out_index;