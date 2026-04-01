select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where negative_group_number >= :selectivity
group by group_number;