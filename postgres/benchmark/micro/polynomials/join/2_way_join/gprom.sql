select
    t1.id :: text || ' ⊗ ' || t2.id :: text
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
order by
    t1.id;