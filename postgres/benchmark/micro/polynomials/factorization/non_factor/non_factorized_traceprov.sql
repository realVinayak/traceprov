select
    string_agg(
        '(' || col_1 :: text || ' ⊗ ' || col_2 :: text || ' ⊗ ' || col_3 :: text || ')',
        ' ⊕ '
    )
from
    traceprov_relation_infer_1;