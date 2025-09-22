SELECT %GUESSED_ID% FROM (PROVENANCE OF (select count(val), z from %TABLE% USE PROVENANCE (id) group by z)) F;
