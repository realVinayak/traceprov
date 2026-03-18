-- using default substitutions
select sum(l_extendedprice * l_discount) as revenue,
	traceprov_log_entry_1(
		2,
		traceprov_agg_key_parallel_offset_1(1, lineitem.rowid::int)
	)
from lineitem
where l_shipdate >= date '1994-01-01'
	and l_shipdate < date '1994-01-01' + interval '1' year
	and l_discount between.06 - 0.01 and.06 + 0.01
	and l_quantity < 24;