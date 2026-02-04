select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT
where id in (select column_1 from LAYER_1)
group by group_number 
having min(min_value) <= :selectivity;