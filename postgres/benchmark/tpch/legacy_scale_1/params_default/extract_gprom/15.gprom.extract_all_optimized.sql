PROVENANCE OF (
	select s_suppkey,
		s_name,
		s_address,
		s_phone,
		total_revenue
	from supplier USE PROVENANCE (s_suppkey),
		revenue0
	where s_suppkey = supplier_no
		and total_revenue = (
			select max(total_revenue)
			from revenue0
		)
	order by s_suppkey
);