select
    column_0 :: text || ' ⊗ ' || column_1 :: text || ' ⊗ ' || column_2 :: text || ' ⊗ ' || column_3 :: text as poly
from
    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 1 :: int);