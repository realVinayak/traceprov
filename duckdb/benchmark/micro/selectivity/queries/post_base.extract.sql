SELECT PROV_DATA__TABLE__1__000__000__RANDOM_ID
FROM (
        PROVENANCE OF (
            select min(min_value) as min_over_group,
                group_number
            from data_table_ROW_COUNT_random USE PROVENANCE (id)
            group by group_number
            having min(min_value) <= :selectivity
            order by min_over_group
        )
    );