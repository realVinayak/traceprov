select avg(val), z from skew_1_1_num_1000000 group by z having avg(val) > 50;
