select
    min(min_value) as min_over_group,
    group_number,
    formula(provenance(), 'data_1_000_000_id_map')
from data_table_1_000_000
where negative_group_number >= :selectivity
group by group_number;