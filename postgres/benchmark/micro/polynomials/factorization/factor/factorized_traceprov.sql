select
    (
        select
            '(' || string_agg(col_3 :: text, ' ⊕ ') || ')'
        from
            traceprov_relation_infer_1
    ) || ' ⊗ ' || (
        select
            '(' || string_agg(col_3 :: text, ' ⊕ ') || ')'
        from
            traceprov_relation_infer_2
    ) || ' ⊗ ' || (
        select
            '(' || string_agg(col_3 :: text, ' ⊕ ') || ')'
        from
            traceprov_relation_infer_3
    );