SELECT Count(*)
FROM   (SELECT *
        FROM   (SELECT *
                FROM   (SELECT Count(*) AS cnt,
                               BLOCK
                        FROM   crimes
                        GROUP  BY BLOCK) f1) f2
        WHERE  cnt > 10000) f3; 