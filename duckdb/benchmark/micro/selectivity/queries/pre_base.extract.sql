SELECT PROV_DATA__TABLE__1__000__000__RANDOM_ID
FROM (
        PROVENANCE OF (
            select min(min_value) as min_over_group,
                group_number
            from data_table_ROW_COUNT_random USE PROVENANCE (id)
            where negative_group_number >= :selectivity
            group by group_number
        )
    );