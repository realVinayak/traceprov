select t_9.out_index as column_0,
    t_0.in_index as column_1
from lineage_view(QID, 9) t_9
    join lineage_view(QID, 7) t_7 on t_9.in_index = t_7.out_index
    join lineage_view(QID, 3) t_3 on t_7.in_index = t_3.out_index
    join lineage_view(QID, 0) t_0 on t_3.in_index = t_0.out_index;