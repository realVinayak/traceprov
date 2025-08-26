SELECT *
FROM   (SELECT Count(*) AS c,
               district,
               ward,
               block,
               community_area,
               beat
        FROM   crimes
        GROUP  BY district,
                  ward,
                  block,
                  community_area,
                  beat) f
ORDER  BY c DESC
LIMIT  5; 