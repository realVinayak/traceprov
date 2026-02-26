#ifndef _TP_COMMON_UTILS_
#define _TP_COMMON_UTILS_

#define round_up(X) (X == 1 ? 1 : (1 << (64 - __builtin_clzl(X - 1))))
#define Max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define Min(X, Y) (((X) < (Y)) ? (X) : (Y))

#endif