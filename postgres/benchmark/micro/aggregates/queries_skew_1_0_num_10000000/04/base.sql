select avg(val), z from skew_1_0_num_10000000 group by z having avg(val) > 50;
