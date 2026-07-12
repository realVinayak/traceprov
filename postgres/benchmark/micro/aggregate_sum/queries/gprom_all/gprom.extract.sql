SELECT 
    prov_skew__1__0__num__1000000_id
FROM (
    PROVENANCE OF (
        select sum(val), z from skew_SKEW_VALUE_num_ROW_COUNT USE PROVENANCE (id) group by z
        )
    ) F;
