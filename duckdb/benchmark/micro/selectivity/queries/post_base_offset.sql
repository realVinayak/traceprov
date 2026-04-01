select * from (
select 
    min(min_value) as min_over_group, 
    group_number 
from data_table_ROW_COUNT_random
group by group_number 
having min(min_value) <= :selectivity
order by min_over_group
) LIMIT 1 OFFSET __TP_OFFSET__