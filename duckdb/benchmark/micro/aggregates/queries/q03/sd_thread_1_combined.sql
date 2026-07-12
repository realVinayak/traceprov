select t_3.out_index,
    t_0.in_index
from lineage_view(QID, 3) t_3
    join lineage_view(QID, 0) t_0 on t_3.in_index = t_0.out_index;