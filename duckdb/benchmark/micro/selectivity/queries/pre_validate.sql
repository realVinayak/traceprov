select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
where id in (select column_1 from traceprov_lineage_1)
group by group_number 
order by min_over_group;