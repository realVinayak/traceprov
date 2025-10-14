CREATE OR REPLACE FUNCTION traceprov_infer_8a3aa4e1(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT p_partkey INTEGER,OUT s_suppkey INTEGER,OUT l_orderkey INTEGER,OUT l_linenumber INTEGER,OUT o_orderkey INTEGER,OUT c_custkey INTEGER,OUT n1_nationkey INTEGER,OUT n2_nationkey INTEGER,OUT r_regionkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;