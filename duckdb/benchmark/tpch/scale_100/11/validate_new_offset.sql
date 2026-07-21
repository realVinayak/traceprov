-- using default substitutions


select
	ps_partkey,
	sum(ps_supplycost * ps_availqty) as value
from
	partsupp,
	supplier,
	nation
where
	ps_suppkey = s_suppkey
	and s_nationkey = n_nationkey
    and (partsupp.rowid, supplier.rowid, nation.rowid) in (
        select
            (eval(column_1, column_1_1),
            column_2,
            column_3)
        FROM
            LAYER_1_%OUT_ID%
    )
group by
	ps_partkey having
		sum(ps_supplycost * ps_availqty) > (
			select
				sum(ps_supplycost * ps_availqty) * 0.0000010000
			from
				partsupp,
				supplier,
				nation
			where
				ps_suppkey = s_suppkey
				and s_nationkey = n_nationkey
				and (partsupp.rowid, supplier.rowid, nation.rowid) in (
					select (
							column_1,
							column_2,
							column_3
						)
					FROM LAYER_2_%OUT_ID%
				)
		)
order by
	value desc;
