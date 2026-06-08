select *
from lineage_view(1, 15) t_15
    join lineage_view(1, 12) t_12 on t_12.out_index = t_15.in_index
    join lineage_view(1, 6) t_6 on t_6.out_index = t_12.in_index