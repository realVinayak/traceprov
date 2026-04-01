select * from
(select distinct b as a from  polynomial_table_0 where id <= NUM)
cross JOIN 
(select distinct b as a from  polynomial_table_1 where id <= NUM)
cross JOIN 
(select distinct b as a from  polynomial_table_2 where id <= NUM)
