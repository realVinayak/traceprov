AC_INIT([traceprov],[1.0])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CXX
LT_INIT
CFLAGS="-Wall -g -O3"
CXXFLAGS="-Wall -g -std=c++11 -O3"
AC_CONFIG_FILES([Makefile])
TRACEPROV_PAGE_SIZE="$(gcc -g get_page_size.c -o get_page_size.o && ./get_page_size.o)"
TRACEPROV_CUR_DIR="$(pwd)"
AC_ARG_WITH([smokedduck_include],
    AS_HELP_STRING([--with-smokedduck_include=PATH], [sets smokedduck includedir]),
    [smokedduck_include_path="$withval"],
    [smokedduck_include_path=""]
)
AC_ARG_WITH([smokedduck],
    AS_HELP_STRING([--with-smokedduck=PATH], [sets smokedduck pathdir]),
    [smokedduck="$withval"],
    [smokedduck=""]
)
AC_ARG_ENABLE([sd_mode],
  AS_HELP_STRING([--enable-sd_mode], [enable foo support]),
  [sd_mode=$enableval],
  [sd_mode=no]
)
AC_SUBST([SMOKEDDUCK_INCUDE_PATH], ["$smokedduck_include_path"])
AC_SUBST([SMOKEDDUCK_PATH], ["$smokedduck"])
AC_SUBST([TRACEPROV_PG_INCLUDE_DIR])
AC_SUBST([TRACEPROV_PAGE_SIZE])
AC_SUBST([TRACEPROV_CUR_DIR])
AM_CONDITIONAL([ENABLE_SD_MODE], [test "x$sd_mode" = "xyes"])
AC_OUTPUT