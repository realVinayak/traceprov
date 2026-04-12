select *,
    count(id) over (
        ORDER BY z ROWS BETWEEN UNBOUNDED PRECEDING and CURRENT ROW
    )
from skew_1_0_num_NUM;