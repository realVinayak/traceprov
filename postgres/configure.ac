AC_INIT([traceprov],[1.0])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CXX
LT_INIT
CFLAGS="-Wall -g -O3"
CXXFLAGS="-Wall -g -std=c++11 -O3"
AC_CONFIG_FILES([Makefile])
TRACEPROV_PG_INCLUDE_DIR="$(pg_config --includedir-server)"
TRACEPROV_PAGE_SIZE="$(gcc -g get_page_size.c -o get_page_size.o && ./get_page_size.o)"
AC_ARG_WITH([smokedduck],
    AS_HELP_STRING([--with-smokedduck=PATH], [enable smokedduck support using PATH]),
    [smokedduck_path="$withval"],
    [smokedduck_path=""]
)
AC_ARG_WITH([smokedduck_include],
    AS_HELP_STRING([--with-smokedduck_include=PATH], [sets smokedduck includedir]),
    [smokedduck_include_path="$withval"],
    [smokedduck_include_path=""]
)
AM_CONDITIONAL([ENABLE_SMOKEDDUCK], [test -n "$smokedduck_path"])
AC_SUBST([SMOKEDDUCK_PATH], ["$smokedduck_path"])
AC_SUBST([SMOKEDDUCK_INCUDE_PATH], ["$smokedduck_include_path"])
AC_SUBST([TRACEPROV_PG_INCLUDE_DIR])
AC_SUBST([TRACEPROV_PAGE_SIZE])
AC_OUTPUT