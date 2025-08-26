--use sql
SELECT F0."AGGR_0" AS "count(1)"
FROM (
SELECT count(1) AS "AGGR_0"
FROM (
SELECT F0."cnt" AS "cnt", F0."block" AS "block"
FROM (
SELECT F0."cnt" AS "cnt", F0."block" AS "block"
FROM (
SELECT F0."AGGR_0" AS "cnt", F0."GROUP_0" AS "block"
FROM (
SELECT count(1) AS "AGGR_0", F0."block" AS "GROUP_0"
FROM "crimes" AS F0
WHERE ((((F0."beat" >= 111) AND (F0."beat" < 112)) OR ((F0."beat" >= 122) AND (F0."beat" < 123))) OR ((F0."beat" >= 1651) AND (F0."beat" < 1652)) OR ((F0."beat" >= 833) AND (F0."beat" < 834)) )
AND (((F0."district" >= 1) AND (F0."district" < 2)) OR ((F0."district" >= 16) AND (F0."district" < 17)) OR ((F0."district" >= 8) AND (F0."district" < 9)))
AND (((F0."ward" >= 41) AND (F0."ward" < 43)) OR ((F0."ward" >= 13) AND (F0."ward" < 14)) OR ((F0."ward" >= 18) AND (F0."ward" < 19)) OR ((F0."ward" >= 34) AND (F0."ward" < 35)) )
AND (((F0."community_area" >= 32) AND (F0."community_area" < 33)) OR ((F0."community_area" >= 76) AND (F0."community_area" < 77)) OR ((F0."community_area" >= 65) AND (F0."community_area" < 66)) OR ((F0."community_area" >= 70) AND (F0."community_area" < 71)) )
GROUP BY F0."block") F0) F0) F0
WHERE (F0."cnt" > 10000)) F0) F0;
