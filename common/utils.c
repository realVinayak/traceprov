// Generic implementation of psprintf (dynamic format)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"


char *tp_psprintf(const char * format, ...){
    va_list args;
    va_list args2;

    va_start(args, format);
    va_copy(args2, args);
    const int final_size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (final_size < 0)
        EXIT_WITH_MESSAGE("Error encoding!");
    
    char *formatted = malloc(final_size + 1);

    memset(formatted, 0, final_size + 1);

    // va_start(args2, format);
    vsnprintf(formatted, final_size + 1, format, args2);
    va_end(args2);

    return formatted;
}