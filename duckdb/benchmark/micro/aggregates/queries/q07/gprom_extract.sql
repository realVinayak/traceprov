SELECT PROV_SKEW__1__0__NUM__1000000_ID
FROM (
        PROVENANCE OF (
            select count(*) as c,
                z
            from skew_1_0_num_ROW_COUNT USE PROVENANCE (id)
            group by z
            ORDER BY c desc
            limit 10
        )
    );