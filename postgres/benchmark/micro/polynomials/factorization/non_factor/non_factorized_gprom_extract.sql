PROVENANCE WITH SEMIRING COMBINER NX OF (
    select
        b
    from
        polynomial_table_0 has provenance (id)
        join polynomial_table_1 has provenance (id) on polynomial_table_0.b = polynomial_table_1.b
        join polynomial_table_2 has provenance (id) on polynomial_table_1.b = polynomial_table_2.b
);