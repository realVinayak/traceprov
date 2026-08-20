PROVENANCE WITH SEMIRING COMBINER NX OF (
    select
        1
    from
        skew_1_0_num_1000 has provenance (id)
        CROSS JOIN polynomial_table_control_0 has provenance (id)
);