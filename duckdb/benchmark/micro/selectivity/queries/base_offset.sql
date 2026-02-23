select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
group by group_number 
having min(min_value) = (__TP_OFFSET__ + 1)