-- using default substitutions


select
	sum(l_extendedprice) / 7.0 as avg_yearly
from
	lineitem,
	part
where
	p_partkey = l_partkey
	and p_brand = 'Brand#23'
	and p_container = 'MED BOX'
	and l_quantity < (
		select
			0.2 * avg(l_quantity)
		from
			lineitem
		where
			l_partkey = p_partkey
			and (lineitem.rowid) in (select opid_9_lineitem from LAYER_1)
	)
	and (lineitem.rowid, part.rowid) in (select opid_12_lineitem, opid_13_part from LAYER_1)
