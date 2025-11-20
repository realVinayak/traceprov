DROP TYPE traceprov_ptr_type CASCADE;

CREATE FUNCTION traceprov_ptr_type_in(cstring)
    RETURNS traceprov_ptr_type
    AS '$libdir/__FILE__', 'traceprov_ptr_type_in'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_out(traceprov_ptr_type)
    RETURNS cstring
    AS '$libdir/__FILE__', 'traceprov_ptr_type_out'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_recv(internal)
    RETURNS traceprov_ptr_type
    AS '$libdir/__FILE__', 'traceprov_ptr_type_recv'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_send(traceprov_ptr_type)
    RETURNS bytea
    AS '$libdir/__FILE__', 'traceprov_ptr_type_send'
    LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE traceprov_ptr_type(
    internallength = 8,
    input = traceprov_ptr_type_in,
    output = traceprov_ptr_type_out,
    receive = traceprov_ptr_type_recv,
    send = traceprov_ptr_type_send,
    alignment = double,
    PASSEDBYVALUE
);

-- Sort operators.
CREATE FUNCTION traceprov_ptr_type_lt(traceprov_ptr_type, traceprov_ptr_type) RETURNS bool
    AS '$libdir/__FILE__', 'traceprov_ptr_type_lt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_gt(traceprov_ptr_type, traceprov_ptr_type) RETURNS bool
    AS '$libdir/__FILE__', 'traceprov_ptr_type_gt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_le(traceprov_ptr_type, traceprov_ptr_type) RETURNS bool
    AS '$libdir/__FILE__', 'traceprov_ptr_type_le' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_eq(traceprov_ptr_type, traceprov_ptr_type) RETURNS bool
    AS '$libdir/__FILE__', 'traceprov_ptr_type_eq' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_ge(traceprov_ptr_type, traceprov_ptr_type) RETURNS bool
    AS '$libdir/__FILE__', 'traceprov_ptr_type_ge' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR < (
   leftarg = traceprov_ptr_type, rightarg = traceprov_ptr_type, procedure = traceprov_ptr_type_lt,
   commutator = > , negator = >= ,
   restrict = scalarltsel, join = scalarltjoinsel
);
CREATE OPERATOR <= (
   leftarg = traceprov_ptr_type, rightarg = traceprov_ptr_type, procedure = traceprov_ptr_type_le,
   commutator = >= , negator = > ,
   restrict = scalarlesel, join = scalarlejoinsel
);
CREATE OPERATOR = (
   leftarg = traceprov_ptr_type, rightarg = traceprov_ptr_type, procedure = traceprov_ptr_type_eq,
   commutator = = ,
   -- leave out negator since we didn't create <> operator
   -- negator = <> ,
   restrict = eqsel, join = eqjoinsel
);
CREATE OPERATOR >= (
   leftarg = traceprov_ptr_type, rightarg = traceprov_ptr_type, procedure = traceprov_ptr_type_ge,
   commutator = <= , negator = < ,
   restrict = scalargesel, join = scalargejoinsel
);
CREATE OPERATOR > (
   leftarg = traceprov_ptr_type, rightarg = traceprov_ptr_type, procedure = traceprov_ptr_type_gt,
   commutator = < , negator = <= ,
   restrict = scalargtsel, join = scalargtjoinsel
);

CREATE FUNCTION traceprov_ptr_type_abs_cmp(traceprov_ptr_type, traceprov_ptr_type) RETURNS int4
    AS '$libdir/__FILE__', 'traceprov_ptr_type_cmp' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS traceprov_ptr_type_abs_ops
    DEFAULT FOR TYPE traceprov_ptr_type USING btree AS
        OPERATOR        1       < ,
        OPERATOR        2       <= ,
        OPERATOR        3       = ,
        OPERATOR        4       >= ,
        OPERATOR        5       > ,
        FUNCTION        1       traceprov_ptr_type_abs_cmp(traceprov_ptr_type, traceprov_ptr_type);

-- Both of the below don't cause a function invocation.
create cast (traceprov_ptr_type as bigint) without function as implicit;
create cast (bigint as traceprov_ptr_type) without function as implicit;