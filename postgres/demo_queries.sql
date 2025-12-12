-- UNION
select var from table_count_2_1_bigint UNION ALL select var from table_count_2_2_bigint UNION ALL select var from table_count_2_3_bigint;
select var from table_count_2_1_bigint UNION ALL select var from table_count_2_2_bigint UNION ALL select var from table_count_2_3_bigint UNION ALL select var from table_count_2_4_bigint;
(select var from table_count_2_1_bigint UNION ALL select var from table_count_2_2_bigint) UNION ALL (select var from table_count_2_3_bigint UNION ALL select var from table_count_2_4_bigint);

-- UNION ALL
select var from table_count_2_1_bigint UNION select var from table_count_2_2_bigint;
select var from table_count_2_1_bigint UNION select var from table_count_2_2_bigint UNION select var from table_count_2_3_bigint;

-- INTERSECT
select var from table_count_2_1_bigint INTERSECT select var from table_count_2_2_bigint;
select var from table_count_2_1_bigint INTERSECT select var from table_count_2_2_bigint INTERSECT select var from table_count_2_3_bigint;

-- INTERSECT ALL
select var from table_count_2_1_bigint INTERSECT ALL select var from table_count_2_2_bigint;
select var from table_count_2_1_bigint INTERSECT ALL select var from table_count_2_2_bigint INTERSECT ALL select var from table_count_2_3_bigint;

-- INTERSECT OF UNION ALL
(select var from table_count_2_1_bigint UNION ALL select var from table_count_2_2_bigint) INTERSECT (select var from table_count_2_3_bigint UNION ALL select var from table_count_2_4_bigint);
(select var from table_count_2_1_bigint UNION ALL select var from table_count_2_2_bigint) INTERSECT (select var from table_count_2_3_bigint UNION ALL select var from table_count_2_4_bigint) INTERSECT (select var from table_count_2_5_bigint UNION ALL select var from table_count_2_6_bigint);


-- INTERSECT OF UNION
(select var from table_count_2_1_bigint UNION select var from table_count_2_2_bigint) INTERSECT (select var from table_count_2_3_bigint UNION select var from table_count_2_4_bigint);
(select var from table_count_2_1_bigint UNION select var from table_count_2_2_bigint) INTERSECT (select var from table_count_2_3_bigint UNION select var from table_count_2_4_bigint) INTERSECT (select var from table_count_2_5_bigint UNION select var from table_count_2_6_bigint);

-- UNION OF INTERSECT
-- There is a bug below (the base relations seem to get lost)
(select var from table_count_2_1_bigint INTERSECT select var from table_count_2_2_bigint) UNION (select var from table_count_2_3_bigint INTERSECT select var from table_count_2_4_bigint);
-- (select var from table_count_2_1_bigint INTERSECT select var from table_count_2_2_bigint) UNION (select var from table_count_2_3_bigint INTERSECT select var from table_count_2_4_bigint);


-- EXCEPT
select var from table_count_2_1_bigint EXCEPT select var from table_count_2_2_bigint where var = 1;
(select var from (select var, id from table_count_2_2_bigint UNION select var, id from table_count_2_2_bigint) f) EXCEPT select var from table_count_2_2_bigint where var = 1;
