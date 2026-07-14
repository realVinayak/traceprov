select id,
    sum(id) over (
        PARTITION BY group_GROUP_NUM
    )
from skew_1_0_num_ROW_COUNT;