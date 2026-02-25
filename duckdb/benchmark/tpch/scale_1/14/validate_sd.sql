	-- using default substitutions


select
	100.00 * sum(case
		when p_type like 'PROMO%'
			then l_extendedprice * (1 - l_discount)
		else 0
	end) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue
from
	lineitem,
	part
where
	l_partkey = p_partkey
	and lineitem.rowid in (select iid from LAYER_1_SD where "table" = 0)
	and part.rowid in (select iid from LAYER_1_SD where "table" = 1)
