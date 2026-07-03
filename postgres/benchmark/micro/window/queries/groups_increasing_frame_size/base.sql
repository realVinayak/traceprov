select *,
    count(id) over (
        ORDER BY z GROUPS BETWEEN UNBOUNDED PRECEDING and CURRENT ROW
    )
from skew_1_0_num_NUM;