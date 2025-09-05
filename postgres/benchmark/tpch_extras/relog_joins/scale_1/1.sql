
select count(*), l_linenumber from 
    (select l_orderkey as l_ok_1, count(*) from lineitem group by l_orderkey) f 
    join lineitem on l_ok_1 = l_orderkey
    group by l_linenumber;
