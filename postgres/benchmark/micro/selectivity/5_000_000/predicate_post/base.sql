select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_5_000_000
group by group_number 
having min(min_value) <= :selectivity;