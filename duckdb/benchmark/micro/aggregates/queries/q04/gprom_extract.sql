select PROV_SKEW__1__0__NUM__1000000_ID
FROM (
        PROVENANCE OF (
            select avg(val),
                z
            from skew_1_0_num_ROW_COUNT USE PROVENANCE (id)
            group by z
            having avg(val) > 50
        )
    );