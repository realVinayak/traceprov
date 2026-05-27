-- using default substitutions
select prov_supplier_s__suppkey,
	prov_lineitem_l__orderkey,
	prov_orders_o__orderkey,
	prov_nation_n__nationkey,
	prov_lineitem_1_l__orderkey
FROM (
		PROVENANCE OF (
			select s_name,
				count(*) as numwait
			from supplier USE PROVENANCE (s_suppkey),
				lineitem USE PROVENANCE (l_orderkey) l1,
				orders USE PROVENANCE (o_orderkey),
				nation USE PROVENANCE (n_nationkey)
			where s_suppkey = l1.l_suppkey
				and o_orderkey = l1.l_orderkey
				and o_orderstatus = 'F'
				and l1.l_receiptdate > l1.l_commitdate
				and exists (
					select *
					from lineitem USE PROVENANCE (l_orderkey) l2
					where l2.l_orderkey = l1.l_orderkey
						and l2.l_suppkey <> l1.l_suppkey
				)
				and not exists (
					select *
					from lineitem USE PROVENANCE (l_orderkey) l3
					where l3.l_orderkey = l1.l_orderkey
						and l3.l_suppkey <> l1.l_suppkey
						and l3.l_receiptdate > l3.l_commitdate
				)
				and s_nationkey = n_nationkey
				and n_name = 'SAUDI ARABIA'
			group by s_name
			order by numwait desc,
				s_name
			LIMIT 100
		)
	);