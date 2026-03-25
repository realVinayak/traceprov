SELECT prov_supplier_s__suppkey,
	prov_lineitem_l__orderkey,
	prov_lineitem_l__linenumber,
	prov_orders_o__orderkey,
	prov_customer_c__custkey,
	prov_nation_n__nationkey,
	prov_nation_1_n__nationkey
FROM (
		PROVENANCE OF (
			select supp_nation,
				cust_nation,
				l_year,
				sum(volume) as revenue
			from (
					select n1.n_name as supp_nation,
						n2.n_name as cust_nation,
						date_part('YEAR', l_shipdate::date) as l_year,
						l_extendedprice * (1 - l_discount) as volume
					from supplier USE PROVENANCE (s_suppkey),
						lineitem USE PROVENANCE (l_orderkey, l_linenumber),
						orders USE PROVENANCE (o_orderkey),
						customer USE PROVENANCE (c_custkey),
						nation USE PROVENANCE (n_nationkey) n1,
						nation USE PROVENANCE (n_nationkey) n2
					where s_suppkey = l_suppkey
						and o_orderkey = l_orderkey
						and c_custkey = o_custkey
						and s_nationkey = n1.n_nationkey
						and c_nationkey = n2.n_nationkey
						and (
							(
								n1.n_name = 'FRANCE'
								and n2.n_name = 'GERMANY'
							)
							or (
								n1.n_name = 'GERMANY'
								and n2.n_name = 'FRANCE'
							)
						)
						and l_shipdate >= '1995-01-01'
						and l_shipdate <= '1996-12-31'
				) as shipping
			group by supp_nation,
				cust_nation,
				l_year
			order by supp_nation,
				cust_nation,
				l_year
		)
	);
-- select
-- 	supp_nation,
-- 	cust_nation,
-- 	l_year,
-- 	sum(volume) as revenue
-- from
-- 	(
-- 		select
-- 			n1.n_name as supp_nation,
-- 			n2.n_name as cust_nation,
-- 			data_part('YEAR', l_shipdate) as l_year,
-- 			l_extendedprice * (1 - l_discount) as volume
-- 		from
-- 			supplier,
-- 			lineitem,
-- 			orders,
-- 			customer,
-- 			nation n1,
-- 			nation n2
-- 		where
-- 			s_suppkey = l_suppkey
-- 			and o_orderkey = l_orderkey
-- 			and c_custkey = o_custkey
-- 			and s_nationkey = n1.n_nationkey
-- 			and c_nationkey = n2.n_nationkey
-- 			and (
-- 				(n1.n_name = 'FRANCE' and n2.n_name = 'GERMANY')
-- 				or (n1.n_name = 'GERMANY' and n2.n_name = 'FRANCE')
-- 			)
-- 			and l_shipdate >= '1995-01-01'
-- 			and l_shipdate <= '1996-12-31'
-- 	) as shipping
-- group by
-- 	supp_nation,
-- 	cust_nation,
-- 	l_year
-- order by
-- 	supp_nation,
-- 	cust_nation,
-- 	l_year;