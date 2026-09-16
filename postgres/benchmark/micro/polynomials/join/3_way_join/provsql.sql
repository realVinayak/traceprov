select
    sr_formula(
        provenance(),
        'zipf_factor_mapping_skew_1_0_num_NUM_2'
    )
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
    JOIN polynomial_table_control_1 as t3 ON t2.id = t3.id
order by
    t1.id;