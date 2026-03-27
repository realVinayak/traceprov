DROP SEQUENCE IF EXISTS prov_ids cascade;
CREATE SEQUENCE prov_ids;

-- for phase 1: add tuid columns

CREATE MATERIALIZED VIEW region_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM region;

CREATE MATERIALIZED VIEW nation_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM nation;

CREATE MATERIALIZED VIEW part_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM part;

CREATE MATERIALIZED VIEW supplier_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM supplier;

CREATE MATERIALIZED VIEW partsupp_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM partsupp;

CREATE MATERIALIZED VIEW customer_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM customer;

CREATE MATERIALIZED VIEW orders_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM orders;

CREATE MATERIALIZED VIEW lineitem_1 AS
  SELECT nextval('prov_ids')::int tuid, * FROM lineitem;


-- create indexes as on origin tables

CREATE INDEX ON region_1 (R_REGIONKEY);
CREATE INDEX ON nation_1 (N_NATIONKEY);
CREATE INDEX ON part_1 (P_PARTKEY);
CREATE INDEX ON supplier_1 (S_SUPPKEY);
CREATE INDEX ON partsupp_1 (PS_PARTKEY, PS_SUPPKEY);
CREATE INDEX ON customer_1 (C_CUSTKEY);
CREATE INDEX ON orders_1 (O_ORDERKEY);
CREATE INDEX ON lineitem_1 (L_ORDERKEY, L_LINENUMBER);

DROP INDEX IF EXISTS nation_fkey_region_1;
DROP INDEX IF EXISTS supplier_fkey_nation_1;
DROP INDEX IF EXISTS partsupp_fkey_part_1;
DROP INDEX IF EXISTS partsupp_fkey_supplier_1;
DROP INDEX IF EXISTS customer_fkey_nation_1;
DROP INDEX IF EXISTS orders_fkey_customer_1;
DROP INDEX IF EXISTS lineitem_fkey_partsupp_1;
DROP INDEX IF EXISTS lineitem_fkey_orders_1;

CREATE INDEX nation_fkey_region_1 ON NATION_1 (N_REGIONKEY);
CREATE INDEX supplier_fkey_nation_1 ON SUPPLIER_1 (S_NATIONKEY);
CREATE INDEX partsupp_fkey_part_1 ON PARTSUPP_1 (PS_PARTKEY);
CREATE INDEX partsupp_fkey_supplier_1 ON PARTSUPP_1 (PS_SUPPKEY);
CREATE INDEX customer_fkey_nation_1 ON CUSTOMER_1 (C_NATIONKEY);
CREATE INDEX orders_fkey_customer_1 ON ORDERS_1 (O_CUSTKEY);
CREATE INDEX lineitem_fkey_partsupp_1 ON LINEITEM_1 (L_PARTKEY, L_SUPPKEY);
CREATE INDEX lineitem_fkey_orders_1 ON LINEITEM_1 (L_ORDERKEY);



-- for phase 2: fill all cells with numbers sequentially 

CREATE MATERIALIZED VIEW region_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] R_REGIONKEY
       , array[nextval('prov_ids')::int] R_NAME     
       , array[nextval('prov_ids')::int] R_COMMENT
  FROM region_1;

CREATE MATERIALIZED VIEW nation_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] N_NATIONKEY
       , array[nextval('prov_ids')::int] N_NAME     
       , array[nextval('prov_ids')::int] N_REGIONKEY
       , array[nextval('prov_ids')::int] N_COMMENT  
  FROM nation_1;

CREATE MATERIALIZED VIEW part_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] P_PARTKEY    
       , array[nextval('prov_ids')::int] P_NAME       
       , array[nextval('prov_ids')::int] P_MFGR       
       , array[nextval('prov_ids')::int] P_BRAND      
       , array[nextval('prov_ids')::int] P_TYPE       
       , array[nextval('prov_ids')::int] P_SIZE       
       , array[nextval('prov_ids')::int] P_CONTAINER  
       , array[nextval('prov_ids')::int] P_RETAILPRICE
       , array[nextval('prov_ids')::int] P_COMMENT    
  FROM part_1;

CREATE MATERIALIZED VIEW supplier_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] S_SUPPKEY  
       , array[nextval('prov_ids')::int] S_NAME     
       , array[nextval('prov_ids')::int] S_ADDRESS  
       , array[nextval('prov_ids')::int] S_NATIONKEY
       , array[nextval('prov_ids')::int] S_PHONE    
       , array[nextval('prov_ids')::int] S_ACCTBAL  
       , array[nextval('prov_ids')::int] S_COMMENT  
  FROM supplier_1;

CREATE MATERIALIZED VIEW partsupp_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] PS_PARTKEY   
       , array[nextval('prov_ids')::int] PS_SUPPKEY   
       , array[nextval('prov_ids')::int] PS_AVAILQTY  
       , array[nextval('prov_ids')::int] PS_SUPPLYCOST
       , array[nextval('prov_ids')::int] PS_COMMENT   
  FROM partsupp_1;

CREATE MATERIALIZED VIEW customer_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] C_CUSTKEY   
       , array[nextval('prov_ids')::int] C_NAME      
       , array[nextval('prov_ids')::int] C_ADDRESS   
       , array[nextval('prov_ids')::int] C_NATIONKEY 
       , array[nextval('prov_ids')::int] C_PHONE     
       , array[nextval('prov_ids')::int] C_ACCTBAL   
       , array[nextval('prov_ids')::int] C_MKTSEGMENT
       , array[nextval('prov_ids')::int] C_COMMENT   
  FROM customer_1;

CREATE MATERIALIZED VIEW orders_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] O_ORDERKEY     
       , array[nextval('prov_ids')::int] O_CUSTKEY      
       , array[nextval('prov_ids')::int] O_ORDERSTATUS  
       , array[nextval('prov_ids')::int] O_TOTALPRICE   
       , array[nextval('prov_ids')::int] O_ORDERDATE    
       , array[nextval('prov_ids')::int] O_ORDERPRIORITY
       , array[nextval('prov_ids')::int] O_CLERK        
       , array[nextval('prov_ids')::int] O_SHIPPRIORITY 
       , array[nextval('prov_ids')::int] O_COMMENT      
  FROM orders_1;

CREATE MATERIALIZED VIEW lineitem_2 AS
  SELECT tuid
       , array[nextval('prov_ids')::int] L_ORDERKEY    
       , array[nextval('prov_ids')::int] L_PARTKEY     
       , array[nextval('prov_ids')::int] L_SUPPKEY     
       , array[nextval('prov_ids')::int] L_LINENUMBER  
       , array[nextval('prov_ids')::int] L_QUANTITY    
       , array[nextval('prov_ids')::int] L_EXTENDEDPRICE
       , array[nextval('prov_ids')::int] L_DISCOUNT    
       , array[nextval('prov_ids')::int] L_TAX         
       , array[nextval('prov_ids')::int] L_RETURNFLAG  
       , array[nextval('prov_ids')::int] L_LINESTATUS  
       , array[nextval('prov_ids')::int] L_SHIPDATE    
       , array[nextval('prov_ids')::int] L_COMMITDATE  
       , array[nextval('prov_ids')::int] L_RECEIPTDATE 
       , array[nextval('prov_ids')::int] L_SHIPINSTRUCT
       , array[nextval('prov_ids')::int] L_SHIPMODE    
       , array[nextval('prov_ids')::int] L_COMMENT     
  FROM lineitem_1;


-- create indexes on tuid

CREATE INDEX ON region_2 (tuid);
CREATE INDEX ON nation_2 (tuid);
CREATE INDEX ON part_2 (tuid);
CREATE INDEX ON supplier_2 (tuid);
CREATE INDEX ON partsupp_2 (tuid);
CREATE INDEX ON customer_2 (tuid);
CREATE INDEX ON orders_2 (tuid);
CREATE INDEX ON lineitem_2 (tuid);

ANALYZE;


-- Logging functions, etc.
\i ../basic-examples/setup.sql
