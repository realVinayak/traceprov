select *,
    max(id + z) over (
        ORDER BY z GROUPS BETWEEN 3 PRECEDING and 2 FOLLOWING
    )
from skew_1_0_num_NUM;