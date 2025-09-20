select avg(val), z from %TABLE% group by z having avg(val) > 50;
