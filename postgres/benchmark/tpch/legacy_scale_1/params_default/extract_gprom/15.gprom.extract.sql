SELECT
	prov_supplier_s__suppkey,
	prov_lineitem_l__orderkey,
	prov_lineitem_l__linenumber,
	prov_lineitem_1_l__orderkey,
	prov_lineitem_1_l__linenumber
FROM (
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
	)
);