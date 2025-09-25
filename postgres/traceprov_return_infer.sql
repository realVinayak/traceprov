CREATE OR REPLACE FUNCTION traceprov_infer(IN integer, IN integer, IN integer,
    OUT f1 integer, OUT f2 integer, OUT f3 integer, OUT f4 integer)
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_infer_time(IN integer, IN integer, IN integer)
    RETURNS BIGINT
    AS '$libdir/__FILE__', 'traceprov_infer_time'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_sync_time(IN integer)
    RETURNS BIGINT
    AS '$libdir/__FILE__', 'traceprov_sync_time'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_layer_stat(
        IN  integer,
        IN  integer,
        OUT is_main_worker INT,
        OUT worker_id INT,
        OUT layer_id INT,
        OUT num_pk_records INT,
        OUT layer_size INT,
        OUT num_groups INT,
        OUT layer_number INT,
        OUT record_padding INT,
        OUT layer_fd INT
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_layer_stat'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
