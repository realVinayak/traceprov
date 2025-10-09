CREATE OR REPLACE FUNCTION traceprov_infer_70ce0282(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT c_custkey INTEGER,OUT o_orderkey INTEGER,OUT l_orderkey INTEGER,OUT l_linenumber INTEGER,OUT s_suppkey INTEGER,OUT n_nationkey INTEGER,OUT r_regionkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;