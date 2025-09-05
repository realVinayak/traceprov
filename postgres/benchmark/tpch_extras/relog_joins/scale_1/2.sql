select count(*), o_custkey from orders join 
    (select l_orderkey, count(*) from lineitem group by l_orderkey) f
    on f.l_orderkey = o_orderkey
    group by o_custkey;
