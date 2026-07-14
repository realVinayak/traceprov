select id,
    sum(id) over (
        ORDER BY z GROUPS BETWEEN UNBOUNDED PRECEDING and CURRENT ROW
    )
from skew_1_0_num_ROW_COUNT;