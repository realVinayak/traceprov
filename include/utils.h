#ifndef _TP_COMMON_UTILS_
#define _TP_COMMON_UTILS_
#include <stdio.h>
#include <stdlib.h>

#define round_up(X) (X == 1 ? 1 : (1 << (64 - __builtin_clzl(X - 1))))
#define Max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define Min(X, Y) (((X) < (Y)) ? (X) : (Y))

static void EXIT_WITH_MESSAGE(const char *MSG) {
    printf("%s", MSG);
    fflush(stdout);
    exit(1);
}

#define failSafeWrite(file, buff, length) { \
    size_t written = fwrite(buff, length, 1, file); \
    if (written != 1){ \
        EXIT_WITH_MESSAGE("Error dumping graph!"); \
    } \
}

#define FAIL_SAFE_WRITE_INT(FILE, VALUE, TYPE) do { \
    int32 value = VALUE; \
    if (sizeof(TYPE) != sizeof(int32)) {  \
       EXIT_WITH_MESSAGE("macro only for int!"); \
    } \
    failSafeWrite(FILE, &value, sizeof(int32)); \
} while(0); \

#define failSafeRead(file, buff, length) { \
    const size_t readValues = fread(buff, length, 1, file); \
    if (readValues != 1){ \
        EXIT_WITH_MESSAGE("Error reading graph!"); \
    } \
} \

char *tp_psprintf(const char * format, ...);

#endif