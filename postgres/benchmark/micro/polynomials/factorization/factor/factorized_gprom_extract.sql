SELECT
    x."PROV" || ' ⊗ ' || y."PROV" || ' ⊗ ' || z."PROV"
FROM
    (
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                1
            from
                polynomial_table_0 has provenance (id)
        )
    ) x,
    (
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                1
            from
                polynomial_table_1 has provenance (id)
        )
    ) y,
    (
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                1
            from
                polynomial_table_2 has provenance (id)
        )
    ) z;