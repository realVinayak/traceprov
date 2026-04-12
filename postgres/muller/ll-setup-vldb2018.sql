CREATE SCHEMA IF NOT EXISTS prov;

--------------------------------------------------------------------------------
-- Provenance Data: Meta Information for Visualizer
--------------------------------------------------------------------------------

-- function calls
drop table if exists call cascade;
create table call ( id int not null,
                    funcn varchar(255) not null
                    );
alter table call add primary key (id);


-- parameters / return values of function calls
-- (return values are no expressions but a plain variable name)
drop table if exists var cascade;
create table var (  id serial not null,
                    name varchar(255) not null,
                    isarg bool not null,
                    call int not null
                    );
alter table var add primary key (id);
alter table var add foreign key (call) references call (id);
create index idx_var_call on var (call);


-- shape of the variables
-- supports nesting of lists/dictionaries/tuples
-- supports json-encoded scalars like int/str/bool
drop table if exists shape cascade;
create table shape (    id serial not null,
                        contid int,       -- null if referenced from var
                        idx varchar(255), -- null if referenced from var
                        isAtom bool not null,
                        value text,       -- null if value is null
                        var int not null
                        );
alter table shape add primary key (id);
alter table shape add foreign key (contid) references shape (id);
alter table shape add foreign key (var) references var (id);
create index idx_shape_var on shape (var);


-- main function and result
insert into call values (1, 'main');
insert into var(name, isArg, call) values ('res', False, 1);



--------------------------------------------------------------------------------
-- Logs: Table Definitions, Logging- and Reading-Functions
--------------------------------------------------------------------------------

-- generates fresh, unique tuple ids and provenance data ids
--drop sequence tuids_seq cascade;
create sequence if not exists tuids_seq start with 1;
select setval('tuids_seq', 1, false);
select setval('tuids_seq', currval('prov_ids'), true);


-- refresh all counters and tables
drop function truncateLogs() cascade;
create or replace function truncateLogs()
    RETURNS void AS 
$$
    truncate logFilter;
    truncate logJoin;
    truncate logAggregation;
    truncate logOrderBy;
    truncate logWindow;
    truncate logCase;
    truncate logUnion;
$$ LANGUAGE sql;

-- ANALYZE logs
drop function analyzeLogs() cascade;
create or replace function analyzeLogs()
    RETURNS void AS 
$$
    ANALYZE logFilter;
    ANALYZE logJoin;
    ANALYZE logAggregation;
    ANALYZE logOrderBy;
    ANALYZE logWindow;
    ANALYZE logCase;
    ANALYZE logUnion;
$$ LANGUAGE sql;

-- return total log size in bytes
drop function logSize() cascade;
create or replace function logSize()
    RETURNS numeric AS 
$$
    SELECT  sum(pg_relation_size(rel)) as "total size"
  FROM  (VALUES ('logAggregation'),
                ('logCase'),
                ('logFilter'), 
                ('logJoin'), 
                ('logOrderBy'),
                ('logUnion'),
                ('logWindow')
      ) rs(rel)
$$ LANGUAGE sql;


--------------------------------------------------------------------------------
-- utility

DROP FUNCTION IF EXISTS report_read_tuid(text, anyelement) CASCADE;
CREATE FUNCTION report_read_tuid(tab text, tuid anyelement)
  RETURNS anyelement AS
$$
BEGIN
  raise notice 'Tried to write tuids-duplicate to ''%'', read ''%'' instead.', tab, tuid;
  return tuid;
END;
$$
LANGUAGE plpgsql;


--------------------------------------------------------------------------------
-- Filter: SF₁W

-- if a tuid is put into this log, the tuple passes the filter
drop table if exists logFilter cascade;
create table logFilter (location int not null,
                        tuid int not null);
alter table logFilter add primary key (location, tuid);


drop function writeFilter(v_location int, v_tuid int) cascade;
create or replace function writeFilter(v_location int, v_tuid int)
    returns int as 
$$
    insert into logFilter (location, tuid)
        values (v_location, v_tuid)
        on conflict do nothing;
    select v_tuid
$$ LANGUAGE sql;


drop function readFilter(v_location int, v_tuid int) cascade;
create or replace function readFilter(v_location int, v_tuid int)
    RETURNS TABLE(tuid int) AS 
$$
    select fi.tuid as tuid
      from logFilter fi
     where fi.location=v_location
       and fi.tuid=v_tuid
$$ LANGUAGE sql STABLE;


/* old: PL/PgSQL function cannot be inlined

drop function readFilter(v_location int) cascade;
create or replace function readFilter(v_location int)
    RETURNS TABLE(tuid int) AS $$
begin
    RETURN QUERY (
        select fi.tuid as tuid
          from logFilter fi
         where fi.location=v_location
    );
end;
$$ LANGUAGE plpgsql;
*/

--------------------------------------------------------------------------------
-- Join: SFₓW

-- input are multiple tuids (input tables)
-- output is a single, fresh tuid
drop table if exists logJoin cascade;
create table logJoin (location int not null,
                      tuid int not null,
                      tuids int[] not null);
alter table logJoin alter column tuid set default nextval('tuids_seq');
alter table logJoin add primary key (location, tuid); -- Q: drop column location?
-- Q: indexs on tuids[1], instead?
-- CREATE INDEX ON logJoin (location, (tuids[1]));
-- unique tuids for ON CONFLICT DO NOTHING (e.g. non-correlated subquery evaluated more than once)  
alter table logJoin add constraint logjoin_location_tuids_key unique (location, tuids);



-- non-efficient version for n-way joins (inefficient inlining, causing nested loops join, even with VARIADIC)
-- see efficient implementations below
drop function readJoin(v_location int, VARIADIC v_tuids int[]) cascade;
create or replace function readJoin(v_location int, v_tuids int[])
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids=v_tuids
$$ LANGUAGE sql STABLE;


-- VARIADIC style: each joined table gives one tuid argument
drop function writeJoin(v_location int, VARIADIC v_tuids int[]) cascade;
create or replace function writeJoin(v_location int, VARIADIC v_tuids int[])
    returns int as 
-- $$
--     -- insert into logJoin (location, tuids)
--     --     values (v_location, v_tuids)
--     --     -- on conflict do nothing
--     --     returning tuid

--     WITH ins(tuid) AS
--         (
--             INSERT INTO logJoin (tuid, location, tuids)
--                 VALUES (DEFAULT, v_location, v_tuids)
--                 ON CONFLICT DO NOTHING
--                 RETURNING tuid
--         )
--     SELECT COALESCE((SELECT tuid FROM ins), 
--                     (SELECT tuid --report_read_tuid('logJoin',tuid) 
--                      FROM   logJoin l
--                      WHERE  l.location = v_location
--                         AND l.tuids = v_tuids))

-- -- -- alternatively:
-- --     WITH ins(tuid) AS 
-- --         (
-- --             INSERT INTO logJoin (tuid, location, tuids) 
-- --                 VALUES (DEFAULT, v_location, v_tuids) 
-- --                 ON CONFLICT DO NOTHING 
-- --                 RETURNING tuid
-- --         ) 
-- --     SELECT tuid FROM ins 
-- --         UNION ALL 
-- --     SELECT  report_read_tuid('logJoin',tuid) 
-- --     FROM    logJoin l
-- --     WHERE   l.location = v_location
-- --         AND l.tuids = v_tuids

-- $$ LANGUAGE SQL;
-- -- alternatively: catching the exception in plpgsql:
$$
DECLARE
    v_tuid int;
BEGIN
    insert into logJoin (location, tuids)
        values (v_location, v_tuids)
        returning tuid into v_tuid;
        return v_tuid;
EXCEPTION
    WHEN UNIQUE_VIOLATION THEN
        RETURN readJoin(v_location, v_tuids);
END;
$$ LANGUAGE PLPGSQL;


-- specialized version for 1-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
$$ LANGUAGE sql STABLE;

-- specialized version for 2-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
$$ LANGUAGE sql STABLE;

-- specialized version for 3-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
$$ LANGUAGE sql STABLE;

-- specialized version for 4-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
$$ LANGUAGE sql STABLE;

-- specialized version for 5-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
           and j.tuids[5]=v_tuid_5
$$ LANGUAGE sql STABLE;

-- specialized version for 6-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
           and j.tuids[5]=v_tuid_5
           and j.tuids[6]=v_tuid_6
$$ LANGUAGE sql STABLE;

-- specialized version for 7-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
           and j.tuids[5]=v_tuid_5
           and j.tuids[6]=v_tuid_6
           and j.tuids[7]=v_tuid_7
$$ LANGUAGE sql STABLE;

-- specialized version for 8-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int, v_tuid_8 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int, v_tuid_8 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
           and j.tuids[5]=v_tuid_5
           and j.tuids[6]=v_tuid_6
           and j.tuids[7]=v_tuid_7
           and j.tuids[8]=v_tuid_8
$$ LANGUAGE sql STABLE;

-- specialized version for 9-way joins (efficient inlining)
drop function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int, v_tuid_8 int, v_tuid_9 int) cascade;
create or replace function readJoin(v_location int, v_tuid_1 int, v_tuid_2 int, v_tuid_3 int, v_tuid_4 int, v_tuid_5 int, v_tuid_6 int, v_tuid_7 int, v_tuid_8 int, v_tuid_9 int)
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
           and j.tuids[2]=v_tuid_2
           and j.tuids[3]=v_tuid_3
           and j.tuids[4]=v_tuid_4
           and j.tuids[5]=v_tuid_5
           and j.tuids[6]=v_tuid_6
           and j.tuids[7]=v_tuid_7
           and j.tuids[8]=v_tuid_8
           and j.tuids[9]=v_tuid_9
$$ LANGUAGE sql STABLE;


-- left outer joins
/* test: log only join columns 
drop function writeJoinLeft(v_location int, v_tuid_1 int, v_tuid_2 int) cascade;
create or replace function writeJoinLeft(v_location int, v_tuid_1 int, v_tuid_2 int)
    returns int as 
$$
    with ins(tuid) as
        (
            insert into logJoin (location, tuids)
                (select v_location, ARRAY[v_tuid_1, v_tuid_2] 
                 where  v_tuid_2 IS NOT NULL)
                returning tuid
        )
    select coalesce((select tuid from ins), v_tuid_1)
$$ LANGUAGE SQL;
*/

drop function readJoinLeft(v_location int, v_tuid_1 int) cascade;
create or replace function readJoinLeft(v_location int, v_tuid_1 int)
    RETURNS TABLE(tuid int, right int) AS 
$$
        select j.tuid, j.tuids[2]
          from logJoin j
         where j.location=v_location
           and j.tuids[1]=v_tuid_1
$$ LANGUAGE sql STABLE;

/* old versions: no inlining

drop table if exists logJoin cascade;
create table logJoin (location int not null,
                      tuid int not null,
                      tuids int[] not null);

drop function writeJoin(v_location int, v_tuids int[]) cascade;
create or replace function writeJoin(v_location int, v_tuids int[])
    returns int as $$
declare
    v_tuid int;
begin
    insert into tupleIds(tableName, sysctid)
        values ('join:'||v_location, null)
        returning id
        into v_tuid;

    insert into logJoin (location, tuid, tuids)
        values (v_location, v_tuid, v_tuids);

    return v_tuid;
end;
$$ LANGUAGE plpgsql;

drop function readJoin(v_location int, v_tuids int[]) cascade;
create or replace function readJoin(v_location int, v_tuids int[])
    RETURNS TABLE(tuid int) AS 
$$
        select j.tuid
          from logJoin j
         where j.location=v_location
           and j.tuids=v_tuids
$$ LANGUAGE sql STABLE;
*/



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

drop function md5(int[]) cascade;
create or replace function md5(int[])
returns text as $$select md5($1::text);$$ language sql immutable;

drop function if exists raise_collision_exception() cascade;
CREATE OR REPLACE FUNCTION raise_collision_exception()
RETURNS int AS $$
BEGIN
  RAISE EXCEPTION 'Unexpected collision unique md5()-index in logAggregation_location_tuids_key';
END;
$$ LANGUAGE plpgsql;

-- alter table logAggregation add constraint logAggregation_location_tuids_key unique (location, tuids);
create unique index logAggregation_location_tuids_key on logAggregation (location, (md5(tuids)));
-- Q: speacial index for v_rowTuid=any(l.tuids)?


drop function writeAggregation(v_location int, v_tuids int[]) cascade;
create or replace function writeAggregation(v_location int, v_tuids int[])
    returns int as 
-- $$
--     -- insert into logAggregation (location, tuids, tuid)
--     --     values (v_location, coalesce(v_tuids, array[]::int[]), DEFAULT)
--     --     -- on conflict do nothing
--     --     returning tuid

--     WITH ins(tuid) AS
--         (
--             INSERT INTO logAggregation (tuid, location, tuids)
--             VALUES (DEFAULT, v_location, v_tuids)
--             ON CONFLICT DO NOTHING
--             RETURNING tuid
--         )
--     SELECT COALESCE((SELECT tuid FROM ins), 
--                     (SELECT tuid --report_read_tuid('logAggregation',tuid) 
--                      FROM   logAggregation l
--                      WHERE  l.location = v_location
--                         AND md5(l.tuids) = md5(v_tuids)
--                         AND l.tuids = v_tuids),
--                     raise_collision_exception())

--         -- with ins(tuid) as (insert into log values (v_location, v_tuids) on conflict do nothing returning tuid) select tuid from ins union all select report_read_tuid('log',tuid) from logJoin where (location,tuid)=(v_location, v_tuids)
-- $$ LANGUAGE sql;
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

drop function readAggregation(v_location int, v_rowTuid int) cascade;
create or replace function readAggregation(v_location int, v_rowTuid int)
    RETURNS TABLE(tuid int) AS 
$$
        select l.tuid as tuid
          from logAggregation l
         where l.location=v_location
           and v_rowTuid=any(l.tuids)
$$ LANGUAGE sql STABLE;


--------------------------------------------------------------------------------
-- Order By: SF₁O

-- logs the tuid and its pos after sorting
drop table if exists logOrderBy cascade;
create table logOrderBy (location int not null,     -- location in query string
                         tuid int not null,         -- a returned tuple
                         sequence serial not null); -- sequence pos. of tuid
alter table logOrderBy add primary key (location, tuid);


drop function writeOrderBy(v_location int, v_tuid int, v_sequence bigint) cascade;
create or replace function writeOrderBy(v_location int, v_tuid int, v_sequence bigint)
    returns int as 
$$
    insert into logOrderBy (location, tuid, sequence)
        values (v_location, v_tuid, v_sequence)
        on conflict do nothing;
    select v_tuid
$$ LANGUAGE sql;

/* old: logging order is not guaranteed to be actual row-order
drop function writeOrderBy(v_location int, v_tuid int) cascade;
create or replace function writeOrderBy(v_location int, v_tuid int)
    returns int as $$
begin
    insert into logOrderBy (location, tuid)
        values (v_location, v_tuid);

    return v_tuid;
end;
$$ LANGUAGE plpgsql;
*/

drop function readOrderBy(v_location int, v_tuid int) cascade;
create or replace function readOrderBy(v_location int, v_tuid int)
    RETURNS TABLE(tuid int, sequence int) AS 
$$
        select l.tuid as tuid,
               l.sequence as sequence
          from logOrderBy l
         where l.location=v_location
           and l.tuid=v_tuid
$$ LANGUAGE sql STABLE;



--------------------------------------------------------------------------------
-- Window Functions: SF₁Win

-- logs the window's parts and order of tuples
drop table if exists logWindow cascade;
create table logWindow (location int not null,
                         tuid int not null,  -- tuid of the tuple
                         part int,       -- part of the tuple
                         pos bigint);     -- rank of the tuple
alter table logWindow add primary key (location, tuid); 
-- Q: better index possible (enhancing WINDOW processing)?


drop function writeWindow(v_location int, v_tuid int, v_part int, v_pos bigint) cascade;
create or replace function writeWindow(v_location int, v_tuid int, v_part int, v_pos bigint)
    returns int as 
$$
    insert into logWindow (location, tuid, part, pos)
        values (v_location, v_tuid, v_part, v_pos)   
        on conflict do nothing;
    select v_tuid
$$ LANGUAGE sql;


drop function readWindow(v_location int, v_tuid int) cascade;
create or replace function readWindow(v_location int, v_tuid int)
    RETURNS TABLE(tuid int, part int, pos bigint) AS 
$$
        select l.tuid as tuid,
               l.part as part,
               l.pos as pos
          from logWindow l
         where l.location=v_location
           and l.tuid=v_tuid
$$ LANGUAGE sql STABLE;



--------------------------------------------------------------------------------
-- Union: SF₁ U SF₁

-- renames tuple ids into unique, fresh ones
-- Q: do we really need renaming? What if tuid occurs twice (same provenance origin!)
drop table if exists logUnion cascade;
create table logUnion (location int not null,
                       old int not null,
                       new int not null);
alter table logUnion alter column new set default nextval('tuids_seq');
alter table logUnion add primary key (new); -- Q: realy necessary?
alter table logUnion add constraint logUnion_location_old_key unique (location, old);


-- old:
-- drop table if exists logFresh cascade;
-- create table logFresh (location int not null,
--                        old int not null,
--                        new int not null);

drop function writeUnion(v_location int, v_tuid int) cascade;
create or replace function writeUnion(v_location int, v_tuid int)
    returns int as 
$$
    -- insert into logUnion (location, old, new)
    --     values (v_location, v_tuid, DEFAULT)    
    --     -- on conflict do nothing
    --     returning new

    WITH ins(tuid) AS
        (
            INSERT INTO logUnion (location, old, new)
            VALUES (v_location, v_tuid, DEFAULT)
            ON CONFLICT DO NOTHING
            RETURNING new
        )
    SELECT COALESCE((SELECT tuid FROM ins), 
                    (SELECT l.new --report_read_tuid('logUnion',l.new) 
                     FROM   logUnion l
                     WHERE  l.location = v_location
                        AND l.old = v_tuid))        
$$ LANGUAGE sql;


drop function readUnion(v_location int, tuid int) cascade;
create or replace function readUnion(v_location int, tuid int)
    returns TABLE (tuid int) as 
$$
     select l.new as tuid
       from logUnion l
      where l.location=v_location
        and l.old=tuid
$$ LANGUAGE sql STABLE;



--------------------------------------------------------------------------------
-- Case Expressions: 

-- logs desicions for a case branch
drop table if exists logCase cascade;
create table logCase (location int not null, -- identifies the case expression
                      tuid int not null,     -- tuple
                      branch int not null);  -- which branch was taken?
alter table logCase add primary key (location,tuid);


drop function writeCase(v_location int, v_tuid int, v_branch int) cascade;
create or replace function writeCase(v_location int, v_tuid int, v_branch int)
    returns int as 
$$
    insert into logCase (location, tuid, branch)
        values (v_location, v_tuid, v_branch)
        on conflict do nothing;
    select v_branch
$$ LANGUAGE sql;


drop function readCase(v_location int, tuid int) cascade;
create or replace function readCase(v_location int, v_tuid int)
    returns int as 
$$
    select l.branch
      from logCase l
     where l.location=v_location
       and l.tuid=v_tuid
$$ LANGUAGE sql STABLE;




--------------------------------------------------------------------------------
-- Table Functions

-- privodes tuple-ids (tuid) for all result tuples of a table function (location)
drop table if exists logTblf cascade;
create table logTblf (location  int not null,
                      tuid      int not null);
alter table logTblf alter column tuid set default nextval('tuids_seq');
alter table logTblf add primary key (location, tuid);


drop function writeTblf(v_location int) cascade;
create or replace function writeTblf(v_location int)
    returns int as 
$$
    insert into logTblf (location, tuid)
        values (v_location, DEFAULT)    
        returning tuid     
$$ LANGUAGE sql VOLATILE;


drop function readTblf(v_location int) cascade;
create or replace function readTblf(v_location int)
    returns TABLE (tuid int) as 
$$
     select l.tuid as tuid
       from logTblf l
      where l.location=v_location
$$ LANGUAGE sql STABLE;

--------------------------------------------------------------------------------
-- Y-Provenance
--------------------------------------------------------------------------------

-- standard (distinct) set enforcement is now heuristically performed on toY(),
-- concat_agg(), set() calls and finally provSize(), but not for each |-opereator use.

-- negate all numbers of an integer array
drop function toY(a int[]) cascade;
drop function toY(a anycompatiblearray) cascade;
CREATE OR REPLACE FUNCTION toY(a anycompatiblearray) 
    returns anycompatiblearray as 
$$
    select array(select distinct -abs(unnest) from unnest(a)) --distinct on toY
$$ LANGUAGE sql IMMUTABLE;


-- thobi=> select array(select -unnest from unnest(array[1,2]));
--   array  
-- ---------
--  {-1,-2}


-- set semantics for integer arrays
drop function set(a anycompatiblearray) cascade;
drop function set(a int[]) cascade;
CREATE OR REPLACE FUNCTION set(a anycompatiblearray)
    returns anycompatiblearray as
$$
    select array (select distinct unnest from unnest(a))
$$ LANGUAGE sql IMMUTABLE;

-- set union of two integer arrays
drop function set_cat(a int[], b int[]) cascade;
CREATE OR REPLACE FUNCTION set_cat(a int[], b int[])
    returns int[] as
$$
    select array
            (select unnest from unnest(a)
               union
             select unnest from unnest(b))
$$ LANGUAGE sql IMMUTABLE;

-- set union of two integer arrays
drop function if exists set_cat(a int[], b int[]) cascade;
drop function if exists set_cat(anycompatiblearray, anycompatiblearray) cascade;
CREATE OR REPLACE FUNCTION set_cat(a int[], b int[])
    returns int[] as
$$
    select array
            (select unnest from unnest(a)
               union
             select unnest from unnest(b))
$$ LANGUAGE sql IMMUTABLE;

drop operator if exists |(anycompatiblearray,anycompatiblearray);
DROP OPERATOR IF EXISTS |(int[],int[]);
CREATE OPERATOR | (
    PROCEDURE = array_cat
  , LEFTARG = anycompatiblearray
  , RIGHTARG = anycompatiblearray
  , COMMUTATOR = |
);

-- USE `jsonb` instead of arrays?!

-- marching-squares=# SELECT '{"1": 1, "2" : 2}'::jsonb || '{"2" : 2, "3" :3}'::jsonb;
--          ?column?         
-- --------------------------
--  {"1": 1, "2": 2, "3": 3}
-- (1 row)

-- Time: 0.313 ms
-- marching-squares=# SELECT ARRAY[1,2] | ARRAY[2,3];
--  ?column? 
-- ----------
--  {1,3,2}
-- (1 row)

-- Time: 0.532 ms



--------------------------------------------------------------------------------
-- Provenance Set Functions
--------------------------------------------------------------------------------

drop function if exists empty();
create function empty() returns int[] as
$$
    select array[]::int[];
$$ language sql immutable;

-- concatenate a set of arrays
drop aggregate if exists array_cat_agg(int[]);
drop aggregate if exists array_cat_agg(anycompatiblearray);
CREATE AGGREGATE array_cat_agg(anycompatiblearray) (
  SFUNC=array_cat,
  STYPE=anycompatiblearray,
  COMBINEFUNC=array_cat,
  PARALLEL=SAFE
);

drop aggregate if exists concat_agg(int[]);
drop aggregate if exists concat_agg(anycompatiblearray);
CREATE AGGREGATE concat_agg(anycompatiblearray) (
  SFUNC=array_cat,
  STYPE=anycompatiblearray,
  FINALFUNC=set,
  COMBINEFUNC=array_cat,
  PARALLEL=SAFE
);

-- dummy function (concat_agg already concatenates)
drop function if exists concat(anycompatiblearray);
create function concat(a anycompatiblearray)
    returns anycompatiblearray as
$$
    select a
$$ language sql immutable;

/* ------- try iteratively

create sequence if not exists concat_agg_seq;
select setval('concat_agg_seq', 1, false);

-- TODO indexing of concat_agg
drop table if exists concat_agg;
create table concat_agg (
  grp int,
  value int
);

DROP FUNCTION IF EXISTS concat_agg_transition(bigint, int[]) CASCADE;
CREATE FUNCTION concat_agg_transition(current bigint, value int[])
  RETURNS bigint AS
$$
DECLARE
  current_agg_id bigint;
BEGIN
  current_agg_id := coalesce(current, nextval('concat_agg_seq'));
  INSERT INTO concat_agg (SELECT current_agg_id, unnest(value));
    RETURN current_agg_id;
END;
$$
LANGUAGE plpgsql;

DROP FUNCTION IF EXISTS concat_agg_final(bigint) CASCADE;
CREATE FUNCTION concat_agg_final(current bigint)
  RETURNS int[] AS
$$
DECLARE
  res int[];
BEGIN
  res :=
    (
    SELECT  array_agg(value) 
    FROM  concat_agg
    WHERE grp=current
    );
  DELETE FROM concat_agg WHERE grp=current;
  RETURN res;
END;
$$
LANGUAGE plpgsql;

DROP AGGREGATE IF EXISTS concat_agg(anyelement);
CREATE AGGREGATE concat_agg(int[])
(
  sfunc = concat_agg_transition,
  FINALFUNC = concat_agg_final,
  stype = bigint
);

*/

-- aggregate function / transition for: the() 
-- - for use in normalization step
DROP FUNCTION IF EXISTS the_transition(anyelement, anyelement) CASCADE;
CREATE FUNCTION the_transition(current anyelement, value anyelement)
  RETURNS anyelement AS
$$
BEGIN
  IF current IS NULL THEN
    RETURN value;
  ELSIF current <> value THEN
    RAISE EXCEPTION 'Non-unique values in the(): % vs. %', current, value;
  ELSE
    RETURN value;
  END IF;
END;
$$
LANGUAGE plpgsql;

DROP AGGREGATE IF EXISTS the(anyelement);
CREATE AGGREGATE the(anyelement)
(
  sfunc = the_transition,
  stype = anyelement
);




-- dependency: set()
drop function provSize(a int[]) cascade;
create or replace function provSize(a int[])
    RETURNS int AS 
$$
    SELECT coalesce(array_length(set(a), 1), 0)
$$ LANGUAGE sql STABLE;




/* first implementation of the() - no longer in use, may be used in phase 2

drop function myThe(pIds int[][]);
create or replace function myThe(pIds int[][])
    returns int[] as $$
declare
    res int[];
begin
    res = (select array(select unnest(pIds[1:1])));
    return res; -- return the first "provenance set" (at index: 0)
end;
$$ LANGUAGE plpgsql;
*/

-- 
