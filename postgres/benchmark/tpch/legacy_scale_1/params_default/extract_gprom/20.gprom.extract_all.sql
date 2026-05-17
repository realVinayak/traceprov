-- using default substitutions
PROVENANCE OF (
	select s_name,
		s_address
	from supplier USE PROVENANCE (s_suppkey),
		nation USE PROVENANCE (n_nationkey),
		(
			select ps_suppkey
			from partsupp USE PROVENANCE (ps_partkey, ps_suppkey)
				JOIN (
					select p_partkey
					from part USE PROVENANCE (p_partkey)
					where p_name like 'forest%'
				) f on f.p_partkey = ps_partkey
				JOIN (
					select 0.5 * sum(l_quantity) as computed,
						l_partkey,
						l_suppkey
					from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
					where l_shipdate >= '1994-01-01'
						and l_shipdate < '1995-01-01'
					group by l_partkey,
						l_suppkey
				) g ON (
					ps_partkey = g.l_partkey
					and ps_suppkey = g.l_suppkey
					and ps_availqty > g.computed
				)
			group by ps_suppkey
		) h
	where h.ps_suppkey = s_suppkey
		and s_nationkey = n_nationkey
		and n_name = 'CANADA'
	order by s_name
);
-- select s_name,
-- 	s_address
-- from supplier,
-- 	nation,
-- 	(
-- 		select ps_suppkey
-- 		from partsupp
-- 		JOIN (
-- 				select p_partkey
-- 				from part
-- 				where p_name like 'forest%'
-- 			) f on f.p_partkey = ps_partkey
-- 		JOIN 
-- 		(
-- 			select 0.5 * sum(l_quantity) as computed, l_partkey, l_suppkey
-- 			from lineitem
-- 			where
-- 				l_shipdate >= '1994-01-01'
-- 				and l_shipdate < '1995-01-01'
-- 			group by l_partkey, l_suppkey
-- 		) g
-- 		ON (ps_partkey = g.l_partkey and ps_suppkey = g.l_suppkey and ps_availqty > g.computed)
-- 		group by ps_suppkey
-- 	) h
-- 	where h.ps_suppkey = s_suppkey
-- 	and s_nationkey = n_nationkey
-- 	and n_name = 'CANADA'
-- order by s_name