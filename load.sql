DROP TABLE IF EXISTS crimes;
CREATE TABLE crimes
(
  id bigint,
  case_number VARCHAR(20),
  date DATE,
  block VARCHAR(50),
  iucr VARCHAR(10),
  primary_type VARCHAR(50),
  description VARCHAR(255),
  location_description VARCHAR(255),
  arrest VARCHAR(10),
  domestic VARCHAR(10),
  beat bigint,
  district bigint,
  ward bigint,
  community_area bigint,
  fbi_code VARCHAR(10),
  x_coordinate bigint, 
  y_coordinate bigint,
  year bigint,
  updated_on date,
  latitude FLOAT(53),
  longitude FLOAT(53),
  location VARCHAR(50),
--   historical_wards VARCHAR(255),
--   zip_codes bigint,
--   community_areas VARCHAR(50),
--   census_tracts VARCHAR(50),
--   wards VARCHAR(50),
--   boundaries_zip_codes VARCHAR(50),
--   police_districts VARCHAR(50),
--   police_beats VARCHAR(50),
  PRIMARY KEY (id)
);

\copy crimes FROM '/home/realvinayak123/projects/prov/crimes.csv' DELIMITER ',' CSV HEADER;


-- ALTER TABLE crimes DROP COLUMN historical_wards;
-- ALTER TABLE crimes DROP COLUMN zip_codes;
-- ALTER TABLE crimes DROP COLUMN community_areas;
-- ALTER TABLE crimes DROP COLUMN census_tracts;
-- ALTER TABLE crimes DROP COLUMN wards;
-- ALTER TABLE crimes DROP COLUMN boundaries_zip_codes;
-- ALTER TABLE crimes DROP COLUMN police_districts;
-- ALTER TABLE crimes DROP COLUMN police_beats;

--clean
delete from crimes where community_area is null or ward is null or case_number is null or district is null;

vacuum;
































