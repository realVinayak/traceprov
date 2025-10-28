select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_100_000_000
where negative_group_number >= :selectivity
group by group_number;