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
					part.rowid in (
                        select column_1_1
                        from LAYER_1_%OUT_ID%
                    )
			)
			and ps_availqty > (
				select
					0.5 * sum(l_quantity)
				from
					lineitem
				where
					l_partkey = ps_partkey
                    and lineitem.rowid in (
                        select column_1_1
                        from LAYER_2_%OUT_ID%
                    )
			)
            and partsupp.rowid in (
                select column_1_1
                from LAYER_4_%OUT_ID%
            )
	)
	and s_nationkey = n_nationkey
    and (supplier.rowid, nation.rowid) in (
        select (eval(column_0, column_1), eval(column_2, column_3))
        from
            LAYER_5_%OUT_ID%
    )
order by
	s_name;
