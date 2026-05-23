select t_29.out_index,
    t_0.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 3) t_3 on t_6.rhs_index = t_3.out_index
    join lineage_view(1, 1) t_1 on t_3.lhs_index = t_1.out_index
    join lineage_view(1, 0) t_0 on t_1.in_index = t_0.out_index;
---
-- select t_29.out_index,
--     t_8.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 8) t_8 on t_10.lhs_index = t_8.out_index;
---
-- select t_29.out_index,
--     t_15.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 15) t_15 on t_22.lhs_index = t_15.out_index;
---
-- select t_29.out_index,
--     t_16.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 17) t_17 on t_21.lhs_index = t_17.out_index
--     join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index;
---
-- select t_29.out_index,
--     t_18.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 18) t_18 on t_20.lhs_index = t_18.out_index;
---
-- select t_29.out_index,
--     t_19.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 19) t_19 on t_20.rhs_index = t_19.out_index;
---
-- select t_29.out_index,
--     t_15.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
--     join lineage_view(1, 15) t_15 on t_22.lhs_index = t_15.out_index;
---
-- select t_29.out_index,
--     t_16.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 17) t_17 on t_21.lhs_index = t_17.out_index
--     join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index;
---
-- select t_29.out_index,
--     t_18.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 18) t_18 on t_20.lhs_index = t_18.out_index;
---
-- select t_29.out_index,
--     t_19.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 3) t_3 on t_6.lhs_index = t_3.out_index
--     join lineage_view(1, 7) t_7 on t_3.rhs_index = t_7.out_index
--     join lineage_view(1, 13) t_13 on t_7.in_index = t_13.out_index
--     join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 19) t_19 on t_20.rhs_index = t_19.out_index;
---
select t_29.out_index,
    t_8.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(1, 10) t_10 on t_13.rhs_index = t_10.out_index
    join lineage_view(1, 8) t_8 on t_10.lhs_index = t_8.out_index;
---
-- select t_29.out_index,
--     t_15.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 15) t_15 on t_22.lhs_index = t_15.out_index;
---
-- select t_29.out_index,
--     t_16.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 17) t_17 on t_21.lhs_index = t_17.out_index
--     join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index;
---
-- select t_29.out_index,
--     t_18.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 18) t_18 on t_20.lhs_index = t_18.out_index;
---
-- select t_29.out_index,
--     t_19.in_index
-- from lineage_view(1, 29) t_29
--     join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
--     join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
--     join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
--     join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
--     join lineage_view(1, 10) t_10 on t_13.lhs_index = t_10.out_index
--     join lineage_view(1, 14) t_14 on t_10.rhs_index = t_14.out_index
--     join lineage_view(1, 22) t_22 on t_14.in_index = t_22.out_index
--     join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
--     join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
--     join lineage_view(1, 19) t_19 on t_20.rhs_index = t_19.out_index;
---
select t_29.out_index,
    t_15.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
    join lineage_view(1, 15) t_15 on t_22.lhs_index = t_15.out_index;
-- orders
---
select t_29.out_index,
    t_16.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
    join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
    join lineage_view(1, 17) t_17 on t_21.lhs_index = t_17.out_index
    join lineage_view(1, 16) t_16 on t_17.in_index = t_16.out_index;
--lineitem
---
select t_29.out_index,
    t_18.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
    join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
    join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
    join lineage_view(1, 18) t_18 on t_20.lhs_index = t_18.out_index;
--- supplier
select t_29.out_index,
    t_19.in_index
from lineage_view(1, 29) t_29
    join lineage_view(1, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(1, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(1, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(1, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(1, 22) t_22 on t_13.rhs_index = t_22.out_index
    join lineage_view(1, 21) t_21 on t_22.rhs_index = t_21.out_index
    join lineage_view(1, 20) t_20 on t_21.rhs_index = t_20.out_index
    join lineage_view(1, 19) t_19 on t_20.rhs_index = t_19.out_index;
--nation