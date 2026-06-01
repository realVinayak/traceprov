-- traceprov_lineage_1
select t_10.out_index,
    t_8.in_index as column_1_1
from lineage_view(QID, 10) t_10
    join lineage_view(QID, 8) t_8 on t_10.lhs_index = t_8.out_index;
------------------------------------------
-- traceprov_lineage_2
select t_29.out_index,
    t_15.in_index as column_3,
    --orders
    t_16.in_index as column_2,
    -- lineitem
    t_18.in_index as column_1,
    -- supplier
    t_19.in_index as column_4 --nation
from lineage_view(QID, 29) t_29
    join lineage_view(QID, 28) t_28 on t_29.in_index = t_28.out_index
    join lineage_view(QID, 27) t_27 on t_28.in_index = t_27.out_index
    join lineage_view(QID, 6) t_6 on t_27.in_index = t_6.out_index
    join lineage_view(QID, 13) t_13 on t_6.rhs_index = t_13.out_index
    join lineage_view(QID, 22) t_22 on t_13.rhs_index = t_22.out_index
    join lineage_view(QID, 15) t_15 on t_22.lhs_index = t_15.out_index
    join lineage_view(QID, 21) t_21 on t_22.rhs_index = t_21.out_index
    join lineage_view(QID, 17) t_17 on t_21.lhs_index = t_17.out_index
    join lineage_view(QID, 16) t_16 on t_17.in_index = t_16.out_index
    join lineage_view(QID, 20) t_20 on t_21.rhs_index = t_20.out_index
    join lineage_view(QID, 18) t_18 on t_20.lhs_index = t_18.out_index
    join lineage_view(QID, 19) t_19 on t_20.rhs_index = t_19.out_index;