PROVENANCE OF (
  WITH revenue as (
    select
      l_suppkey as supplier_no,
      sum(l_extendedprice * (1 - l_discount)) AS total_revenue
      from
        lineitem USE PROVENANCE (l_orderkey, l_linenumber)
     where
l_shipdate >=  '1996-01-01'::date
       and l_shipdate < '1996-01-01'::date + '3 month'::interval
     group by
l_suppkey)

  select
    s_suppkey,
    s_name,
    s_address,
    s_phone,
    total_revenue
    from
      supplier USE PROVENANCE (s_suppkey),
      revenue
   where
s_suppkey = supplier_no
     and total_revenue = (
       select
         max(total_revenue)
         from
           revenue
     )
   order by
s_suppkey);