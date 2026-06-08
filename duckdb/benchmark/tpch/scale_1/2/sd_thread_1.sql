-- traceprov_lineage_1
select t_34.out_index,
    t_0.in_index as column_1_1,
    t_1.in_index as column_2_1,
    t_3.in_index as column_3_1,
    t_5.in_index as column_4_1
from lineage_view(1, 34) t_34
    join lineage_view(1, 32) t_32 on t_34.in_index = t_32.out_index
    join lineage_view(1, 28) t_28 on t_32.in_index = t_28.out_index
    join lineage_view(1, 15) t_15 on t_28.in_index = t_15.out_index
    join lineage_view(1, 11) t_11 on t_15.lhs_index = t_11.out_index
    join lineage_view(1, 8) t_8 on t_11.in_index = t_8.out_index
    join lineage_view(1, 6) t_6 on t_8.lhs_index = t_6.out_index
    join lineage_view(1, 4) t_4 on t_6.lhs_index = t_4.out_index
    join lineage_view(1, 2) t_2 on t_4.lhs_index = t_2.out_index
    join lineage_view(1, 0) t_0 on t_2.lhs_index = t_0.out_index
    join lineage_view(1, 1) t_1 on t_2.rhs_index = t_1.out_index
    join lineage_view(1, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(1, 5) t_5 on t_6.rhs_index = t_5.out_index;
--------------------------------------------------------
-- traceprov_lineage_3
select t_34.out_index,
    t_17.in_index as column_2,
    t_18.in_index as column_0,
    t_21.in_index as column_1,
    t_22.in_index as column_3,
    t_23.in_index as column_4
from lineage_view(1, 34) t_34
    join lineage_view(1, 32) t_32 on t_34.in_index = t_32.out_index
    join lineage_view(1, 28) t_28 on t_32.in_index = t_28.out_index
    join lineage_view(1, 15) t_15 on t_28.in_index = t_15.out_index
    join lineage_view(1, 26) t_26 on t_15.rhs_index = t_26.out_index
    join lineage_view(1, 20) t_20 on t_26.lhs_index = t_20.out_index
    join lineage_view(1, 17) t_17 on t_20.lhs_index = t_17.out_index
    join lineage_view(1, 19) t_19 on t_20.rhs_index = t_19.out_index
    join lineage_view(1, 18) t_18 on t_19.in_index = t_18.out_index
    join lineage_view(1, 25) t_25 on t_26.rhs_index = t_25.out_index
    join lineage_view(1, 21) t_21 on t_25.lhs_index = t_21.out_index
    join lineage_view(1, 24) t_24 on t_25.rhs_index = t_24.out_index
    join lineage_view(1, 22) t_22 on t_24.lhs_index = t_22.out_index
    join lineage_view(1, 23) t_23 on t_24.rhs_index = t_23.out_index;