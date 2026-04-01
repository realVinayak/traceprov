select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where id in (select column_1 from LAYER_1_%OUT_ID%)
group by group_number
order by min_over_group;