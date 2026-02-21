select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where rowid in (select "iid" from LAYER_1_SD)
group by group_number 
having min(min_value) <= :selectivity;