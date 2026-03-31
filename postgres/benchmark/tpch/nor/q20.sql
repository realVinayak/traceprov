select  distinct s_name, s_address from supplier, nation, 
(select ps_suppkey from partsupp,     
(select p_partkey from part where p_name like 'green%') p1,
(select l_partkey,l_suppkey, 0.5 * sum(l_quantity) as sum_lq from lineitem 
where l_shipdate >= '1993-01-01' and l_shipdate < '1994-01-01'
group by l_partkey,l_suppkey) l1
where l_partkey = ps_partkey and l_suppkey = ps_suppkey and ps_availqty > sum_lq
and ps_partkey = p_partkey) ps1 
where s_suppkey = ps_suppkey and s_nationkey = n_nationkey and n_name = 'CHINA'
order by s_name;
