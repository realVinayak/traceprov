SELECT supplier.s_suppkey,
	supplier.s_name,
	supplier.s_address,
	supplier.s_phone,
	revenue0.total_revenue
FROM supplier,
	(
		SELECT lineitem.l_suppkey AS supplier_no,
			sum(
				(
					lineitem.l_extendedprice * (1 - lineitem.l_discount)
				)
			) AS total_revenue
		FROM lineitem
		WHERE lineitem.rowid in (
				select column_1_1,
					FROM traceprov_lineage_1
			)
		GROUP BY lineitem.l_suppkey
	) revenue0
WHERE (
		(supplier.s_suppkey = revenue0.supplier_no)
		AND (
			revenue0.total_revenue = (
				SELECT tp_table_1.max
				FROM (
						SELECT max(revenue0_1.total_revenue) AS max
						FROM (
								SELECT lineitem.l_suppkey AS supplier_no,
									sum(
										(
											lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
										)
									) AS total_revenue
								FROM lineitem
								WHERE lineitem.rowid in (
										select column_1_1
										from traceprov_lineage_2
									)
								GROUP BY lineitem.l_suppkey
							) revenue0_1
					) tp_table_1
			)
		)
	)
	and (
		supplier.rowid in (
			select column_0
			FROM traceprov_lineage_5
		)
	)
ORDER BY supplier.s_suppkey;