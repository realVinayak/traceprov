-- using default substitutions
PROVENANCE OF (
	select s_name,
		s_address
	from supplier USE PROVENANCE (s_suppkey),
		nation USE PROVENANCE (n_nationkey)
	where s_suppkey in (
			select ps_suppkey
			from partsupp USE PROVENANCE (ps_partkey)
			where ps_partkey in (
					select p_partkey
					from part USE PROVENANCE (p_partkey)
					where p_name like 'forest%'
				)
				and ps_availqty > (
					select 0.5 * sum(l_quantity)
					from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
					where l_partkey = ps_partkey
						and l_suppkey = ps_suppkey
						and l_shipdate >= '1994-01-01'
						and l_shipdate < '1995-01-01'
				)
		)
		and s_nationkey = n_nationkey
		and n_name = 'CANADA'
	order by s_name
);