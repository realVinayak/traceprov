lib_LTLIBRARIES = libtraceprov.la
libtraceprov_la_SOURCES = traceprov.cpp file_utils.cpp utils.cpp traceprov_duckdb_infer.cpp traceprov_extra_funcs.cpp traceprov_settings.cpp traceprov_derive.cpp

AM_CFLAGS = -I${SMOKEDDUCK_INCUDE_PATH} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
AM_CPPFLAGS = -I${SMOKEDDUCK_INCUDE_PATH} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
AM_CFLAGS += -I../include/ -DTP_USE_PG_LIST=0
AM_CPPFLAGS += -I../include/ -DTP_USE_PG_LIST=0
libtraceprov_la_LIBADD = -lduckdb -ltp_common
libtraceprov_la_LDFLAGS = -L${SMOKEDDUCK_PATH} -L${TRACEPROV_CUR_DIR}/../common/bld/lib -Wl,-rpath,${SMOKEDDUCK_PATH} -Wl,-rpath,${TRACEPROV_CUR_DIR}/../common/bld/lib
bin_PROGRAMS = run_smokedduck
run_smokedduck_SOURCES = run_duckdb_query.cpp
run_smokedduck_LDADD = -lduckdb libtraceprov.la -ltp_common
run_smokedduck_LDFLAGS = -L${SMOKEDDUCK_PATH} -L${TRACEPROV_CUR_DIR}/../common/bld/lib -Wl,-rpath,${SMOKEDDUCK_PATH} -Wl,-rpath,${TRACEPROV_CUR_DIR}/../common/bld/lib
if ENABLE_SD_MODE
AM_CFLAGS += -DTRACEPROV_SD_MODE=1
AM_CPPFLAGS += -DTRACEPROV_SD_MODE=1
endif