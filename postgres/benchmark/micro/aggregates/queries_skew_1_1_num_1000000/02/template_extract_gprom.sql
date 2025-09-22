SELECT prov_skew__1__1__num__1000000_id FROM (PROVENANCE OF (select avg(val), z from skew_1_1_num_1000000 USE PROVENANCE (id) group by z)) F;
