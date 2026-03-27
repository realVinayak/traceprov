
-- generates fresh, unique tuple ids and provenance data ids
--drop sequence tuids_seq cascade;
create sequence if not exists tuids_seq start with 1;


-- input are multiple tuids (constituting group [0..n])
-- output is a single, fresh tuid (the aggregate for that group)
drop table if exists logAggregation cascade;
create table logAggregation (location int not null,
                         tuids int[] not null,  -- all tuids of a certain group
                         tuid int not null);         -- replacement tuid for that group
alter table logAggregation alter column tuid set default nextval('tuids_seq');
alter table logAggregation add primary key (location, tuid);


--------------------------------------------------------------------------------
-- Aggregation and Group By: SF₁G

-- input are multiple tuids (constituting group [0..n])
-- output is a single, fresh tuid (the aggregate for that group)
drop table if exists logAggregation cascade;
create table logAggregation (location int not null,
                         tuids int[] not null,  -- all tuids of a certain group
                         tuid int not null);         -- replacement tuid for that group
alter table logAggregation alter column tuid set default nextval('tuids_seq');
alter table logAggregation add primary key (location, tuid); -- Q: drop column location?



drop function if exists writeAggregation(v_location int, v_tuids int[]) cascade;
create or replace function writeAggregation(v_location int, v_tuids int[])
    returns int as
$$
DECLARE
    v_tuid int;
BEGIN
    insert into logAggregation (location, tuids)
        values (v_location, coalesce(v_tuids, array[]::int[]))
        returning tuid into v_tuid;
        return v_tuid;
EXCEPTION
    WHEN UNIQUE_VIOLATION THEN
        RETURN COALESCE(
                    (SELECT tuid --report_read_tuid('logAggregation',tuid)
                     FROM   logAggregation l
                     WHERE  l.location = v_location
                        AND md5(l.tuids) = md5(v_tuids)
                        AND l.tuids = v_tuids),
                    raise_collision_exception());
END;
$$ LANGUAGE PLPGSQL;

drop function if exists readAggregation(v_location int, v_rowTuid int) cascade;
create or replace function readAggregation(v_location int, v_rowTuid int)
    RETURNS TABLE(tuid int) AS
$$
        select l.tuid as tuid
          from logAggregation l
         where l.location=v_location
           and v_rowTuid=any(l.tuids)
$$ LANGUAGE sql STABLE;


--------------------------------------------------------------------------------
-- usage example (15.1.2026)


-- thobi@noxi:~/tmp$ psql < agg_vldb2018.sql
-- CREATE SEQUENCE
-- NOTICE:  table "logaggregation" does not exist, skipping
-- DROP TABLE
-- CREATE TABLE
-- ALTER TABLE
-- ALTER TABLE
-- DROP TABLE
-- CREATE TABLE
-- ALTER TABLE
-- ALTER TABLE
-- NOTICE:  function writeaggregation(pg_catalog.int4,pg_catalog.int4[]) does not exist, skipping
-- DROP FUNCTION
-- CREATE FUNCTION
-- NOTICE:  function readaggregation(pg_catalog.int4,pg_catalog.int4) does not exist, skipping
-- DROP FUNCTION
-- CREATE FUNCTION
-- thobi@noxi:~/tmp$ psql
-- psql (17.7 (Ubuntu 17.7-0ubuntu0.25.10.1))
-- Type "help" for help.

-- thobi=# select writeAggregation(5, array[10, 11, 12]);
--  writeaggregation
-- ------------------
--                 1
-- (1 row)

-- thobi=# table logaggregation ;
--  location |   tuids    | tuid
-- ----------+------------+------
--         5 | {10,11,12} |    1
-- (1 row)


