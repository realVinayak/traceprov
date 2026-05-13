select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where rowid in (select "iid" from LAYER_1_SD_%OUT_ID%)
group by group_number 
order by min_over_group;