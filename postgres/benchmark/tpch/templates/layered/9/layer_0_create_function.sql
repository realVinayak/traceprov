CREATE OR REPLACE FUNCTION traceprov_infer_918a7564(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT p_partkey INTEGER,OUT s_suppkey INTEGER,OUT l_orderkey INTEGER,OUT l_linenumber INTEGER,OUT ps_partkey INTEGER,OUT ps_suppkey INTEGER,OUT o_orderkey INTEGER,OUT n_nationkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;