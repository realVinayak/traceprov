select 
	s_name,
	count(*) as numwait
from
	supplier,
	(select distinct l_orderkey, l_suppkey, l_receiptdate, l_commitdate from (
select l_orderkey, l_suppkey,l_receiptdate, l_commitdate, bit_or(dateb::bit(1)) as datebc from (
select  l1.l_orderkey, l1.l_suppkey, l1.l_receiptdate, l1.l_commitdate, case when l2.l_receiptdate > l2.l_commitdate then '1'::bit(1) else '0'::bit(1) end as dateb from (select * from lineitem) l1, lineitem l2 where l2.l_orderkey = l1.l_orderkey and l2.l_suppkey <> l1.l_suppkey) l group by l_orderkey, l_suppkey, l_receiptdate, l_commitdate) ll where datebc != '1') l1,
	orders,
	nation
where
	s_suppkey = l1.l_suppkey
	and o_orderkey = l1.l_orderkey
	and o_orderstatus = 'F'
	and l1.l_receiptdate > l1.l_commitdate
	and s_nationkey = n_nationkey
	and n_name = 'CHINA'
group by
	s_name
order by
	numwait desc,
	s_name
LIMIT 100;
