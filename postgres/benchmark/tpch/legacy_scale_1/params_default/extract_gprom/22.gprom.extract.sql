-- using default substitutions

select 
prov_customer_c__custkey,
prov_customer_1_c__custkey
FROM (
	PROVENANCE OF (
		select
			cntrycode,
			count(*) as numcust,
			sum(c_acctbal) as totacctbal
		from
			(
				select
					SUBSTR(c_phone, 1, 2) as cntrycode,
					c_acctbal
				from
					customer USE PROVENANCE (c_custkey)
				where
					SUBSTR(c_phone, 1, 2) in
						('13', '31', '23', '29', '30', '18', '17')
					and c_acctbal > (
						select
							avg(c_acctbal)
						from
							customer USE PROVENANCE (c_custkey)
						where
							c_acctbal > 0.00
							and SUBSTR(c_phone, 1, 2) in
								('13', '31', '23', '29', '30', '18', '17')
					)
					and not exists (
						select
							*
						from
							orders USE PROVENANCE (o_orderkey)
						where
							o_custkey = c_custkey
					)
			) as custsale
		group by
			cntrycode
		order by
			cntrycode
	)
);
