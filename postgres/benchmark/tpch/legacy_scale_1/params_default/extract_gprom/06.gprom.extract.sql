SELECT prov_lineitem_l__orderkey,
	prov_lineitem_l__linenumber
FROM (
		PROVENANCE OF (
			select sum(l_extendedprice * l_discount) as revenue
			from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
			where l_shipdate >= '1994-01-01'
				and l_shipdate < '1995-01-01'
				and l_discount >= 0.05
				and l_discount <= 0.07
				and l_quantity < 24
		)
	);