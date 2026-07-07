select
  PROV_PART_P__PARTKEY,
  PROV_SUPPLIER_S__SUPPKEY,
  PROV_LINEITEM_L__ORDERKEY,
  PROV_PARTSUPP_PS__PARTKEY,
  PROV_ORDERS_O__ORDERKEY,
  PROV_NATION_N__NATIONKEY,
  NATION,
  O_YEAR
FROM
  (
    SELECT
      part.rowid AS PROV_PART_P__PARTKEY,
      supplier.rowid AS PROV_SUPPLIER_S__SUPPKEY,
      lineitem.rowid AS PROV_LINEITEM_L__ORDERKEY,
      partsupp.rowid AS PROV_PARTSUPP_PS__PARTKEY,
      orders.rowid AS PROV_ORDERS_O__ORDERKEY,
      nation.rowid AS PROV_NATION_N__NATIONKEY,
      nation.N_NAME AS NATION,
      DATE_PART('YEAR', CAST((O_ORDERDATE) AS DATE)) AS O_YEAR,
    from
      part,
      supplier,
      lineitem,
      partsupp,
      orders,
      nation
    where
      s_suppkey = l_suppkey
      and ps_suppkey = l_suppkey
      and ps_partkey = l_partkey
      and p_partkey = l_partkey
      and o_orderkey = l_orderkey
      and s_nationkey = n_nationkey
      and p_name like '%green%'
    ORDER BY
      NATION ASC,
      O_YEAR DESC
  )