select col_0,
    col_1,
    col_2,
    concat_agg(array [col_3])
from traceprov_relation_infer_1
group by col_0,
    col_1,
    col_2;