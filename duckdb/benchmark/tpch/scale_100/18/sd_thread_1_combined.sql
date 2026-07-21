-- traceprov_lineage_1
select t_14.out_index,
    t_5.in_index as column_1_1
from lineage_view(QID, 14) t_14
    join lineage_view(QID, 10) t_10 on t_14.in_index = t_10.out_index
    join lineage_view(QID, 8) t_8 on t_10.in_index = t_8.out_index
    join lineage_view(QID, 5) t_5 on t_8.in_index = t_5.out_index;
----------------------------------------------------
-- traceprov_lineage_3
select t_24.out_index,
    t_0.in_index as column_4,
    -- lineitem
    t_1.in_index as column_2,
    -- orders
    t_2.in_index as column_1 -- customer
from lineage_view(QID, 24) t_24
    join lineage_view(QID, 22) t_22 on t_24.in_index = t_22.out_index
    join lineage_view(QID, 19) t_19 on t_22.in_index = t_19.out_index
    join lineage_view(QID, 16) t_16 on t_19.in_index = t_16.out_index
    join lineage_view(QID, 4) t_4 on t_16.lhs_index = t_4.out_index
    join lineage_view(QID, 0) t_0 on t_4.lhs_index = t_0.out_index
    join lineage_view(QID, 3) t_3 on t_4.rhs_index = t_3.out_index
    join lineage_view(QID, 1) t_1 on t_3.lhs_index = t_1.out_index
    join lineage_view(QID, 2) t_2 on t_3.rhs_index = t_2.out_index;