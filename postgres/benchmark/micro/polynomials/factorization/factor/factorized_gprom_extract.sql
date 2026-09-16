SELECT
    x."PROV" || ' ⊗ ' || y."PROV" || ' ⊗ ' || z."PROV"
FROM
    (
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                b
            from
                polynomial_table_0 has provenance (id)
        )
    ) x
    JOIN (
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                b
            from
                polynomial_table_1 has provenance (id)
        )
    ) y ON x.b = y.b
    JOIN(
        PROVENANCE WITH SEMIRING COMBINER NX OF (
            select
                b
            from
                polynomial_table_2 has provenance (id)
        )
    ) z ON z.b = y.b;