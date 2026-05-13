select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where rowid in (select opid_5_data_table_ROW_COUNT from LAYER_1)
group by group_number 
order by min_over_group;