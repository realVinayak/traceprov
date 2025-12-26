lib_LTLIBRARIES = libtraceprov.la libtraceprov_infer.la
libtraceprov_la_SOURCES = traceprov.c file_utils.c utils.c
libtraceprov_infer_la_SOURCES = traceprov_return_infer.cpp file_utils.c utils.c
AM_CFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
AM_CPPFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
libtraceprov_la_LDFLAGS = -avoid-version
libtraceprov_infer_la_LDFLAGS = -avoid-version
if ENABLE_SMOKEDDUCK
bin_PROGRAMS = run_smokedduck
run_smokedduck_SOURCES = run_duckdb_query.cpp
run_smokedduck_CPPFLAGS = -I${SMOKEDDUCK_INCUDE_PATH}
run_smokedduck_LDFLAGS = -L${SMOKEDDUCK_PATH} -Wl,-rpath,${SMOKEDDUCK_PATH}
run_smokedduck_LDADD = -lduckdb 
endif
