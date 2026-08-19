PROVENANCE WITH SEMIRING COMBINER NX OF (
    select
        1
    from
        polynomial_table_0 has provenance (id)
        cross join polynomial_table_1 has provenance (id)
        cross join polynomial_table_2 has provenance (id)
);