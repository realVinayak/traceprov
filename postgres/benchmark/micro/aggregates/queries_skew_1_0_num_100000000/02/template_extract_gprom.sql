SELECT prov_skew__1__0__num__100000000_id FROM (PROVENANCE OF (select avg(val), z from skew_1_0_num_100000000 USE PROVENANCE (id) group by z)) F;
