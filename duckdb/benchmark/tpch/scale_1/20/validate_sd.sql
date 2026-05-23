-- using default substitutions


select
	s_name,
	s_address
from
	supplier,
	nation
where
	s_suppkey in (
		select
			ps_suppkey
		from
			partsupp
		where
			ps_partkey in (
				select
					p_partkey
				from
					part
				where
					part.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 12)
			)
			and ps_availqty > (
				select
					0.5 * sum(l_quantity)
				from
					lineitem
				where
					l_partkey = ps_partkey
					and l_suppkey = ps_suppkey
					and lineitem.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 0)
			)
			and partsupp.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 11)
	)
	and s_nationkey = n_nationkey
	and supplier.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 24)
	and nation.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 25)
order by
	s_name;
