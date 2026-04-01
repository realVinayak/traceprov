select 
    distinct polynomial_table_1.b 
    from polynomial_table_0 
    cross join polynomial_table_1 
    cross join polynomial_table_2 
where polynomial_table_0.id <= NUM and polynomial_table_1.id <= NUM and polynomial_table_2.id <= NUM;