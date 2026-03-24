select * from (select count(*) as c,
    z
from skew_1_0_num_ROW_COUNT
group by z
ORDER BY c
limit 10) LIMIT 1 OFFSET __TP_OFFSET__